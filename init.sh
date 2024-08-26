#!/bin/bash

luajit_dir=$(pwd)/luajit2
luajit_backup_dir=$(pwd)/luajit_backup
patch_dir=$(pwd)/patch

#====================================================================================================
# Init git submodules.
#====================================================================================================
git submodule update --init --recursive

#====================================================================================================
# Save some files that will be replaced after pathing the luajit submodule into backup directory.
#====================================================================================================
rm -rf $luajit_backup_dir
mkdir -p $luajit_backup_dir/src

cp $luajit_dir/src/lj_load.c $luajit_backup_dir/src/lj_load.c
cp $luajit_dir/src/Makefile.dep $luajit_backup_dir/src/Makefile.dep
cp $luajit_dir/src/Makefile $luajit_backup_dir/src/Makefile

#====================================================================================================
# Applying path files.
#====================================================================================================
cp $patch_dir/src/lj_load.c $luajit_dir/src/lj_load.c
cp $patch_dir/src/lj_load_helper.cpp $luajit_dir/src/lj_load_helper.cpp
cp $patch_dir/src/Makefile.dep $luajit_dir/src/Makefile.dep
cp $patch_dir/src/Makefile $luajit_dir/src/Makefile

#====================================================================================================
# Install luajit.
#====================================================================================================
source $(pwd)/install_luajit.sh
