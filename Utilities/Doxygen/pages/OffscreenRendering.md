ParaView and Offscreen Rendering        {#Offscreen}
================================

ParaView is often used to render visualization results. While in most cases,
the results are presented to the users on their screens or monitors, there are
cases when this is not required. For example, if running a batch script to
generate several images, one may not care to see windows flashing in front of
them while the results are being generated. Another use-case is when using a
**pvserver** or **pvrenderserver** to render images remotely and having them
sent back to the client for viewing. The server-side windows are do not serve
any useful purpose in that mode. In other cases, showing a window may not be
possible at all, for example on certain Linux clusters, one may not have access
to an X server at all, in which case on-screen rendering is just not possible
(this is also referred to as headless operating mode).

This page documents the onscreen, offscreen, and headless rendering support in
ParaView.

##Caveats##

In this page, we limit ourselves to a Linux-based system. On Windows and macOS,
offscreen support is available, but true headless operation is currently not
widely used and hence left out of this discussion.

##Terminology##

A brief explanation of the terms:

* **Desktop Qt client**: This refers to the Qt-based GUI. This is launched using
  the **paraview** executable.

* **Command line executables**: This refers to all other ParaView executables,
  e.g. **pvserver**, **pvdataserver**, **pvrenderserver**, **pvpython**,
  and **pvbatch**.

* **Onscreen**: Onscreen refers to the case where the rendering results are
  presented to the users in a viewable window. On Linux, this invariably requires
  a running and accessible X server. The desktop Qt client can only operate in
  on-screen mode and hence needs an accessible X server.

* **Offscreen**: Offscreen simply means that the rendering results are not presented
  to the user in a window. On Linux, this does not automatically imply that an
  accessible X server is not needed. X server may be needed, unless ParaView
  was built with an OpenGL implementation that allows for headless operation.
  This mode is not applicable to the desktop Qt client and is only supported
  by the command line executables.

* **Headless**: Headless rendering automatically implies offscreen rendering.
  In addition, on Linux, it implies that the rendering does not require an
  accessible X server nor will it make any X calls.

**Onscreen** and **offscreen** support is built by default. Thus ParaView
binaries available from paraview.org support these modes. **Headless** support
requires special builds of ParaView which are not compatible with standard
build options. Both version are available as a binary from our downloads page.

##OpenGL Implementations##

ParaView uses OpenGL for rendering. OpenGL is an API specification for 2D/3D
rendering. Many vendors provide implementations of this API, including those
that build GPUs.

For sake of simplicity, let's classify OpenGL implementations as **hardware
(H/W)** and **software (S/W)**. **H/W** includes OpenGL implementations provided by
NVIDIA, ATI, Intel, Apple and others which typically use the system hardware
infrastructure for rendering. The runtime libraries needed for these are
available on the system. **S/W** currently includes Mesa3D -- a software
implementation of the OpenGL standard. Despite the names, H/W doesn't
necessarily imply use of GPUs, nor does S/W imply exclusion of GPUs. Nonetheless
we use this naming scheme as it has been prevalent.

###APIs for Headless Support###

Traditionally, OpenGL implementations are coupled with the window system to
provide an OpenGL context. Thus, they are designed for non-headless operation.
When it comes to headless operation, there are alternative APIs that an
application can use to create the OpenGL context that avoid this dependency on
the window system (or X server for the sake of this discussion).

Currently, ParaView supports two distinct APIs that are available for headless
operation: **EGL** and **OSMesa** (also called **Offscreen Mesa**). It must be
noted that headless support is a rapidly evolving area and changes are expected
in coming months. Modern H/W OpenGL implementations support EGL while S/W
(or Mesa) supports OSMesa. One has to build ParaView with specific CMake flags
changed to enable either of these APIs. Which headless API you choose in your
build depends on which OpenGL implementation you plan to use.

##ParaView Builds##

Before we look at the various ways you can build and use ParaView, let's
summarize relevant CMake options available:

* `VTK_USE_X`: When ON, implies that ParaView can link against X libraries.
  This allows ParaView executables to create on-screen windows, if needed.

  When `VTK_USE_X` is ON, these variables must be specified:
  * `OPENGL_INCLUDE_DIR`: Path to directory containing `GL/gl.h`.
  * `OPENGL_gl_LIBRARY`: Path to `libGL.so`.
  * `OpengL_glu_LIBRARY`: not needed for ParaView; leave empty.
  * `OPENGL_xmesa_INCLUDE_DIR`: not needed for ParaView; leave empty.

* `VTK_OPENGL_HAS_OSMESA`: When ON, implies that ParaView can use OSMesa
  to support headless modes of operation.

  When `VTK_OPENGL_HAS_OSMESA` is ON, these variables must be specified:
  * `OSMESA_INCLUDE_DIR`: Path to containing `GL/osmesa.h`.
  * `OSMESA_LIBRARY`: Path to `libOSMesa.so`

* `VTK_OPENGL_HAS_EGL`: When ON, implies that ParaView can use EGL to support
  headless modes of operation.

  When `VTK_OPENGL_HAS_EGL` is ON, these variables must be specified:
  * `EGL_INCLUDE_DIR`: Path to directory containing `GL/egl.h`.
  * `EGL_LIBRARY`: Path to `libEGL.so`.
  * `EGL_opengl_LIBRARY`: Path to `libOpenGL.so`.

* `PARAVIEW_USE_QT` indicates if the desktop Qt client should be built.

`VTK_USE_X` and `VTK_OPENGL_HAS_OSMESA` are mutually exclusive i.e.
only one of the two can be ON at the same time. This is due to the
fact that  OSMesa does not support on-screen rendering and
VTK's OpenGL selection is at build time.

`VTK_OPENGL_HAS_EGL` and `VTK_OPENGL_HAS_OSMESA` are mutually exclusive i.e.
only one of the two can be ON at the same time.

A few things to note:
* At least one of `VTK_OPENGL_HAS_EGL`, `VTK_OPENGL_HAS_OSMESA` and `VTK_USE_X` must be ON.
* If `VTK_OPENGL_HAS_EGL` or `VTK_OPENGL_HAS_OSMESA` is ON, the build supports headless rendering, otherwise
  `VTK_USE_X` must be ON and the build does not support headless, but can still support offscreen rendering.
* `PARAVIEW_USE_QT=ON` and `VTK_USE_X=OFF` has limited support.
   In this configuration, the command-line tools like `pvserver`, `pvrenderserver` will not depend on X.
   The `paraview` Qt client, however, is not built.
* If `VTK_OPENGL_HAS_EGL` is ON and `VTK_USE_X` is ON, then all the OpenGL and EGL variables should point
  to the system libraries providing both, typically the NVidia libraries.

##Default Rendering Modes##

Depending of the enabled build options, which type of render window the ParaView
executable creates by default also needs some explanation.

* The ParaView desktop Qt client always creates an onscreen window using GLX calls via Qt.
* `pvserver`, `pvrenderserver` and `pvbatch` always create an offscreen render window. If built with headless support,
  it will be an offscreen-headless window. There are a few exceptions where it will create an onscreen window instead:
  - if running in tile-display mode, i.e. `-tdx` or `-tdy` command line options are passed
  - if running in immersive mode e.g. CAVE
  - if `PV_DEBUG_REMOTE_RENDERING` environment is set
  - if `--force-onscreen-rendering` command line option is passed.
* `pvpython` always creates an onscreen render window if built with onscreen support. The following are the exceptions when it
  creates offscreen (or headless if supported) render windows.
  - if `--force-offscreen-rendering` command line option is passed.

`--use-offscreen-rendering` command line option supported by ParaView 5.4 and earlier has now been deprecated and is
interpreted as `--force-offscreen-rendering`.
