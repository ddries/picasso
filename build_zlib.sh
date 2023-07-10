#!/bin/sh

git submodule update --init
cd external/zlib
./configure --static
make
cd ../..
