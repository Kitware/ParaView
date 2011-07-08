#!/bin/bash

cmake \
    -DCMAKE_C_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/cc \
    -DCMAKE_CXX_COMPILER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DCMAKE_LINKER=/opt/cray/xt-asyncpe/3.6/bin/CC \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DPARAVIEW_BUILD_QT_GUI=OFF \
    -DVTK_USE_X=OFF \
    -DVTK_OPENGL_HAS_OSMESA=ON \
    -DOPENGL_INCLUDE_DIR=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-pgi/include \
    -DOPENGL_gl_LIBRARY="" \
    -DOPENGL_glu_LIBRARY=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-pgi/lib/libGLU.a \
    -DOPENGL_xmesa_INCLUDE_DIR=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-pgi/include \
    -DOSMESA_INCLUDE_DIR=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-pgi/include \
    -DOSMESA_LIBRARY=/nics/c/home/bloring/apps/Mesa-7.7-osmesa-pgi/lib/libOSMesa.a \
    -DPARAVIEW_USE_MPI=ON \
    -DMPI_INCLUDE_PATH=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-pgi/include \
    -DMPI_LIBRARY=/opt/cray/mpt/4.0.1/xt/seastar/mpich2-pgi/lib/libmpich.a \
    $*
