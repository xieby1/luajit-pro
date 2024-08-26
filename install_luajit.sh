#!/bin/bash

luajit_dir=$(pwd)/luajit2
install_dir=$(pwd)/luajit2

cd $luajit_dir; make clean; make -j $(nproc); make install PREFIX=$install_dir

