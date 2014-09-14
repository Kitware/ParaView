# How to build an Off-screen ParaViewWeb server

## Introduction

Web servers do not always come with a GPU. In addition, small 3D models can easily be handled by software rendering.
This documentation will focus on how to build ParaView so that it can be used in a headless environment for ParaViewWeb. This only applies for a Linux base system.

## Building the application

### OSMesa

More informations can be found in the [wiki](http://paraview.org/Wiki/ParaView/ParaView_And_Mesa_3D), but this page provides a nice and easy summary.

The Mesa 9.2.2 OSMesa Gallium llvmpipe state-tracker is the preferred Mesa back-end renderer for ParaView and VTK. The following shows how to configure it with system installed LLVM. Our strategy is to configure Mesa with the minimal number of options needed for OSMesa. This greatly simplifies the build, as many of the other drivers/renderers depend on X11 or other libraries. The following set of options are from the Mesa v9.2.2 release. Older or newer releases may require slightly different options. Consult ./configure --help for the details.

    #!/bin/bash

    make -j4 distclean # if in an existing build

    autoreconf -fi

    ./configure \
        CXXFLAGS="-O2 -g -DDEFAULT_SOFTWARE_DEPTH_BITS=31" \
        CFLAGS="-O2 -g -DDEFAULT_SOFTWARE_DEPTH_BITS=31" \
        --disable-xvmc \
        --disable-glx \
        --disable-dri \
        --with-dri-drivers="" \
        --with-gallium-drivers="swrast" \
        --enable-texture-float \
        --disable-shared-glapi \
        --disable-egl \
        --with-egl-platforms="" \
        --enable-gallium-osmesa \
        --enable-gallium-llvm=yes \
        --with-llvm-shared-libs \
        --prefix=/opt/mesa/9.2.2/llvmpipe

    make -j2
    make -j4 install

Some explanation of these options:

* DEFAULT_SOFTWARE_DEPTH_BITS=31
This sets the internal depth buffer precision for the OSMesa rendering context. In our experience, this is necessary to avoid z-buffer fighting during parallel rendering. Note that we have used this in-place of --with-osmesa-bits=32, which sets both depth buffer and color buffers to 32 bit precision. Because of a bug in Mesa, this introduces over 80 ctest regression failures in VTK related to line drawing.

* --enable-texture-float
Floating point textures are disabled by default due to patent restrictions. They must be enabled for many advanced VTK algorithms.

### ParaView

ParaView needs to be built from source in order to take advantage of your OSMesa build.

    $ cd ParaViewWeb
    $ mkdir ParaView
    $ cd ParaView
    $ mkdir build install
    $ git clone git://paraview.org/stage/ParaView.git src
    $ cd src
    $ git submodule update --init
    $ cd ../build
    $ ccmake ../src

        PARAVIEW_ENABLE_PYTHON      ON
        PARAVIEW_BUILD_QT_GUI       OFF
        CMAKE_INSTALL_PREFIX        /.../ParaView/install
        VTK_USE_X                   OFF
        OPENGL_INCLUDE_DIR          /opt/mesa/9.2.2/llvmpipe/include
        OPENGL_gl_LIBRARY
        OPENGL_glu_LIBRARY          /opt/mesa/9.2.2/llvmpipe/lib/libGLU.[so|a]
        VTK_OPENGL_HAS_OSMESA       ON
        OSMESA_INCLUDE_DIR          /opt/mesa/9.2.2/llvmpipe/include
        OSMESA_LIBRARY              /opt/mesa/9.2.2/llvmpipe/lib/libOSMesa.[so|a]

    $ make
    $ make install
