#!/bin/bash

COMP=/opt/intel/Compiler/11.1/038/bin/intel64

cmake \
    -DCMAKE_C_COMPILER=$COMP/icc \
    -DCMAKE_CXX_COMPILER=$COMP/icpc \
    -DCMAKE_LINKER=$COMP/icpc \
    -DCMAKE_CXX_FLAGS=-Wno-deprecated \
    $*

