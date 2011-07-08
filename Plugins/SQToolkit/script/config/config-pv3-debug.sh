#!/bin/bash

cmake \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPARAVIEW_USE_MPI=ON \
    -DVTK_DEBUG_LEAKS=ON \
    -DCMAKE_CXX_FLAGS_DEBUG="-g -Wall" \
    $*

