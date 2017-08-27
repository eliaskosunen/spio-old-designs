#!/bin/bash

./build.sh
cd tests
gdb ../build/tests/tests
cd ..
