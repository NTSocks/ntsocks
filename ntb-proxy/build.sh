#!/bin/bash 

mkdir build 

./autogen.sh 

cd build/
make distclean
../configure 
make -j4

cd ..
