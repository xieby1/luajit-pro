#!/bin/bash

luajit_dir=$(pwd)/luajit2.1
install_dir=$(pwd)/luajit2.1

cd $luajit_dir; make clean; make -j $(nproc); make install PREFIX=$install_dir

