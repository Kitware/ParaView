#!/bin/bash

COMP=/nasa/intel/cce/10.1.021/bin
MPI=/nasa/sgi/mpt/1.26

cmake \
    -DCMAKE_C_COMPILER=$COMP/icc \
    -DCMAKE_CXX_COMPILER=$COMP/icpc \
    -DCMAKE_LINKER=$COMP/icpc \
    -DCMAKE_CXX_FLAGS=-Wno-deprecated \
    -DMPI_INCLUDE_PATH=$MPI/include \
    -DMPI_LIBRARY=$MPI/lib/libmpi.so \
    -DBUILD_SVTK_UTILITIES=ON \
    $*
