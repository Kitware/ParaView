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

### Apache

See [Amazon EC2 AMI instance to run ParaViewWeb](index.html#!/guide/paraviewweb_on_aws_ec2) for more details.
__Only needed for real deployment.__

### Launcher

We provide two implementations of the process launcher. One, however, is already embedded inside ParaView. Hence, we will explain how to configure this one. More information can be read on the Java-based implementation, which can be found [here](index.html#!/guide/jetty_session_manager) or on the python-based implementation, which can be found
[here](index.html#!/guide/py_launcher).


Path to replace:

- __configuration.content__ [OPTIONAL]: Directory that we want to share on the Web.
- __configuration.sessionURL__: Web socket URL that should be used. In case of local setup with no web socket forwarding __ws://${host}:${port}/ws__ can be used. Otherwise in case of Apache doing the forwarding it needs to match the rewrite rule but something like __ws://hostname/proxy?sessionId=${id}__ can be used.
- __configuration.log_dir__: Path where session log files will be stored.
- __properties.python_exec__: Path to pvpython executable of ParaView.
- __properties.pvweb__: Path to the directory that contains all ParaViewWeb python files.
- __properties.data__: Path to the directory to share.

To configure your launcher, you will need to create a configuration file like the following (__config.json__):

    {
     "configuration": {
	     "host" : "localhost",
	     "port" : 8080,
	     "endpoint": "paraview",
	     "content": "/path-to-paraview-www-dir/www",
	     "proxy_file" : "/path-to-share-with-apache/proxy-map.txt",
	     "sessionURL" : "ws://${host}:${port}/ws",
	     "timeout" : 5,
	     "log_dir" : "/path-to-store/logs",
	     "fields" : ["secret", "file"]
    },

    "resources" : [ { "host" : "localhost", "port_range" : [9000, 9100] } ],

    "properties" : {
       "python_exec" : "/Users/seb/work/pvweb/paraview.app/Contents/bin/pvpython",
       "pvweb": "/Users/seb/work/pvweb/paraview.app/Contents/Python/paraview/web",
       "data": "/Users/seb/work/code/ParaView/data/Data"
    },

    "apps" : {
       "pipeline" : {
         "cmd" : [ "${python_exec}", "${pvweb}/pv_web_visualizer.py", "--port", "$port", "--data-dir", "$data" ],
         "ready_line" : "Starting factory"
       },
       "loader" : {
         "cmd" : [ "${python_exec}", "${pvweb}/pv_web_file_loader.py", "--port", "$port", "--data-dir", "$data" ],
         "ready_line" : "Starting factory"
       },
       "data_prober" : {
         "cmd" : [ "${python_exec}", "${pvweb}/pv_web_data_prober.py", "--port", "$port", "--data-dir", "$data"],
         "ready_line" : "Starting factory"
       }
     }
    }

Then, in order to run the launcher, just execute:

    $ ./bin/pvpython /.../vtk/web/launcher.py config.json
