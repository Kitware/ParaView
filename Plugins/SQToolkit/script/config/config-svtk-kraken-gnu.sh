#!/bin/bash

export XTPE_LINK_TYPE=dynamic

MPI_PATH=/opt/cray/mpt/5.0.0/xt/seastar/mpich2-gnu
COMP=/opt/cray/xt-asyncpe/4.0/bin

cmake \
  -DCMAKE_C_COMPILER=$COMP/cc \
  -DCMAKE_CXX_COMPILER=$COMP/CC \
  -DCMAKE_LINKER=$COMP/CC \
  -DPARAVIEW_USE_MPI=ON \
  -DMPI_INCLUDE_PATH=$MPI_PATH/include \
  -DMPI_LIBRARY=$MPI_PATH/lib/libmpich.so \
  $*

