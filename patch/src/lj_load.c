/*
** Load and dump code.
** Copyright (C) 2005-2023 Mike Pall. See Copyright Notice in luajit.h
*/

#include <errno.h>
#include <stdio.h>

#define lj_load_c
#define LUA_CORE

#include "lua.h"
#include "lauxlib.h"

#include "lj_obj.h"
#include "lj_gc.h"
#include "lj_err.h"
#include "lj_buf.h"
#include "lj_func.h"
#include "lj_frame.h"
#include "lj_vm.h"
#include "lj_lex.h"
#include "lj_bcdump.h"
#include "lj_parse.h"

#ifdef LUAJIT_SYNTAX_EXTEND
#include "assert.h"


typedef const char *(* LuaDoStringPtr)(const char*);
char *file_transform(const char *filename, LuaDoStringPtr func);
void string_transform(const char *str, size_t *output_size);
void luaL_openlibs(lua_State *L);

const char *do_lua_stiring(const char *str) {
    static lua_State *L;
    static char init = 0;
    if (init == 0) {
        init = 1;
        L    = luaL_newstate();
        luaL_openlibs(L);
    }

    // Execute the Lua string
    if (luaL_dostring(L, str) != LUA_OK) {
        // If execution fails, get the error message
        const char *errorMsg = lua_tostring(L, -1);
        printf("Error executing Lua code: %s\n", errorMsg);
        
        // Clean up the stack by popping the error message
        lua_pop(L, 1); // Remove the error message from the stack

        lua_close(L); // Close the Lua state
        printf("code_str >>> \n%s\n<<<\n", str);
        assert(0 && "Error executing luaCode");
    }
    
    if (lua_isstring(L, -1)) {
        return lua_tostring(L, -1);
    } else {
        return "";
        // assert(0 && "Error: Lua did not return a string");
    }
}

#endif // LUAJIT_SYNTAX_EXTEND

/* -- Load Lua source code and bytecode ----------------------------------- */

static TValue *cpparser(lua_State *L, lua_CFunction dummy, void *ud)
{
  LexState *ls = (LexState *)ud;
  GCproto *pt;
  GCfunc *fn;
  int bc;
  UNUSED(dummy);
  cframe_errfunc(L->cframe) = -1;  /* Inherit error function. */
  bc = lj_lex_setup(L, ls);
  if (ls->mode) {
    int xmode = 1;
    const char *mode = ls->mode;
    char c;
    while ((c = *mode++)) {
      if (c == (bc ? 'b' : 't')) xmode = 0;
      if (c == (LJ_FR2 ? 'W' : 'X')) ls->fr2 = !LJ_FR2;
    }
    if (xmode) {
      setstrV(L, L->top++, lj_err_str(L, LJ_ERR_XMODE));
      lj_err_throw(L, LUA_ERRSYNTAX);
    }
  }
  pt = bc ? lj_bcread(ls) : lj_parse(ls);
  if (ls->fr2 == LJ_FR2) {
    fn = lj_func_newL_empty(L, pt, tabref(L->env));
    /* Don't combine above/below into one statement. */
    setfuncV(L, L->top++, fn);
  } else {
    /* Non-native generation returns a dumpable, but non-runnable prototype. */
    setprotoV(L, L->top++, pt);
  }
  return NULL;
}

LUA_API int lua_loadx(lua_State *L, lua_Reader reader, void *data,
		      const char *chunkname, const char *mode)
{
  LexState ls;
  int status;
  ls.rfunc = reader;
  ls.rdata = data;
  ls.chunkarg = chunkname ? chunkname : "?";
  ls.mode = mode;
  lj_buf_init(L, &ls.sb);
  status = lj_vm_cpcall(L, NULL, &ls, cpparser);
  lj_lex_cleanup(L, &ls);
  lj_gc_check(L);
  return status;
}

LUA_API int lua_load(lua_State *L, lua_Reader reader, void *data,
		     const char *chunkname)
{
  return lua_loadx(L, reader, data, chunkname, NULL);
}

typedef struct FileReaderCtx {
#ifdef LUAJIT_SYNTAX_EXTEND
  char filename[256]; /* Max 255 + 1 for null terminator. */
  unsigned char is_first_access;
#endif // LUAJIT_SYNTAX_EXTEND
  FILE *fp;
  char buf[LUAL_BUFFERSIZE];
} FileReaderCtx;

static const char *reader_file(lua_State *L, void *ud, size_t *size)
{
  FileReaderCtx *ctx = (FileReaderCtx *)ud;
  UNUSED(L);
  if (feof(ctx->fp)) return NULL;

#ifdef LUAJIT_SYNTAX_EXTEND
  if(ctx->is_first_access == 1) {
    // The file read by LuaJIT is seperated by many parts to avoid stack overflow for some large files.
    ctx->is_first_access = 0;

    char first_line_buffer[256];
    const char* substring = "--[[verilua]]";

    if (fgets(first_line_buffer, sizeof(first_line_buffer), ctx->fp) != NULL) {
      if (strstr(first_line_buffer, substring) != NULL) {
        char *new_file = file_transform(ctx->filename, do_lua_stiring);
        // printf("[Debug]new_file => %s\n", new_file);fflush(stdout);
        fclose(ctx->fp);
        ctx->fp = fopen(new_file, "rb");
        free(new_file);
      } else {
        // The read file did not contains "--[[verilua]]"
      }
      fseek(ctx->fp, 0, SEEK_SET);
    } else {
      assert(0 && "Cannot read the file!");
    }
  }
#endif // LUAJIT_SYNTAX_EXTEND

  *size = fread(ctx->buf, 1, sizeof(ctx->buf), ctx->fp);
  return *size > 0 ? ctx->buf : NULL;
}

LUALIB_API int luaL_loadfilex(lua_State *L, const char *filename,
			      const char *mode)
{
  FileReaderCtx ctx;
  int status;
  const char *chunkname;
  if (filename) {
    ctx.fp = fopen(filename, "rb");
    if (ctx.fp == NULL) {
      lua_pushfstring(L, "cannot open %s: %s", filename, strerror(errno));
      return LUA_ERRFILE;
    }
    chunkname = lua_pushfstring(L, "@%s", filename);
  } else {
    ctx.fp = stdin;
    chunkname = "=stdin";
  }

#ifdef LUAJIT_SYNTAX_EXTEND
  // Save filename to ctx
  if(filename == NULL) {
    assert(0 && "filename is NULL!\n");
  } else {
    snprintf(ctx.filename, sizeof(ctx.filename), "%s", filename);
  }

  // A flag that indicates whether it is the first access to the file.
  ctx.is_first_access = 1;
#endif // LUAJIT_SYNTAX_EXTEND

  status = lua_loadx(L, reader_file, &ctx, chunkname, mode);
  if (ferror(ctx.fp)) {
    L->top -= filename ? 2 : 1;
    lua_pushfstring(L, "cannot read %s: %s", chunkname+1, strerror(errno));
    if (filename)
      fclose(ctx.fp);
    return LUA_ERRFILE;
  }
  if (filename) {
    L->top--;
    copyTV(L, L->top-1, L->top);
    fclose(ctx.fp);
  }
  return status;
}

LUALIB_API int luaL_loadfile(lua_State *L, const char *filename)
{
  return luaL_loadfilex(L, filename, NULL);
}

typedef struct StringReaderCtx {
  const char *str;
  size_t size;
} StringReaderCtx;

static const char *reader_string(lua_State *L, void *ud, size_t *size)
{
  StringReaderCtx *ctx = (StringReaderCtx *)ud;
  UNUSED(L);
  if (ctx->size == 0) return NULL;
  *size = ctx->size;
  ctx->size = 0;

#ifdef LUAJIT_SYNTAX_EXTEND
  string_transform(ctx->str, size);
#endif // LUAJIT_SYNTAX_EXTEND

  return ctx->str;
}

LUALIB_API int luaL_loadbufferx(lua_State *L, const char *buf, size_t size,
				const char *name, const char *mode)
{
  StringReaderCtx ctx;
  ctx.str = buf;
  ctx.size = size;
  return lua_loadx(L, reader_string, &ctx, name, mode);
}

LUALIB_API int luaL_loadbuffer(lua_State *L, const char *buf, size_t size,
			       const char *name)
{
  return luaL_loadbufferx(L, buf, size, name, NULL);
}

LUALIB_API int luaL_loadstring(lua_State *L, const char *s)
{
  return luaL_loadbuffer(L, s, strlen(s), s);
}

/* -- Dump bytecode ------------------------------------------------------- */

LUA_API int lua_dump(lua_State *L, lua_Writer writer, void *data)
{
  cTValue *o = L->top-1;
  uint32_t flags = LJ_FR2*BCDUMP_F_FR2;  /* Default mode for legacy C API. */
  lj_checkapi(L->top > L->base, "top slot empty");
  if (tvisfunc(o) && isluafunc(funcV(o)))
    return lj_bcwrite(L, funcproto(funcV(o)), writer, data, flags);
  else
    return 1;
}

