{ luajit
, fetchFromGitHub
}:
(luajit.override rec {
  version = "v2.1-20240815";
  src = fetchFromGitHub {
    owner = "openresty";
    repo = "luajit2";
    rev = version;
    hash = "sha256-cAmcj6mssDn7G8c+GTEasn4qfxgr4X8ZtWrs5uQhskM=";
  };
}).overrideAttrs (old: {
  postPatch = ''
    cp ${./patch/src/lj_load.c}           src/lj_load.c
    cp ${./patch/src/lj_load_helper.cpp}  src/lj_load_helper.cpp
    cp ${./patch/src/Makefile.dep}        src/Makefile.dep
    cp ${./patch/src/Makefile}            src/Makefile
  '' + old.postPatch;
  buildFlags = [];
})
