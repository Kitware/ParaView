#!/bin/bash

COMP=/opt/apps/intel/11.1/bin/intel64
MPI=/opt/apps/intel11_1/openmpi/1.3.3/
#  -DMPI_LIB=$MPI/lib/libmpi.so
#    -DMPI_INC=$MPI/include
#    -DMPI_COMPILER=$MPI/bin/mpic++ \
   

cmake \
    -DCMAKE_C_COMPILER=$COMP/icc \
    -DCMAKE_CXX_COMPILER=$COMP/icpc \
    -DCMAKE_LINKER=$COMP/icpc \
    -DEXTRA_INTEL_INCLUDES=/opt/apps/intel/11.1/include/intel64/ \
    -DCMAKE_CXX_FLAGS=-Wno-deprecated \
    -DMPI_COMPILER:FILEPATH=/opt/apps/intel11_1/openmpi/1.3.3/bin/mpic++ \
    -DMPI_COMPILE_FLAGS:STRING=\
    -DMPI_EXTRA_LIBRARY:STRING=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-rte.so\;/opt/apps/intel11_1/openmpi/1.3.3/lib/libopen-pal.so\;/usr/lib64/libdl.so\;/usr/lib64/libnsl.so\;/usr/lib64/libutil.so \
    -DMPI_INCLUDE_PATH:STRING=/opt/apps/intel11_1/openmpi/1.3.3/include \
    -DMPI_LIBRARY:FILEPATH=/opt/apps/intel11_1/openmpi/1.3.3/lib/libmpi.so \
    -DMPI_LINK_FLAGS:STRING=-Wl,--export-dynamic \
    -DSVTK_UTILITIES=ON \
    $*

