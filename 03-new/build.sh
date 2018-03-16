#!/bin/bash

if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake -DWERROR=OFF -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Wdev ..
make -j4
cd ../tests
../build/tests/tests
cd ..
