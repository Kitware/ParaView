# Building ParaView

This page describes how to build and install ParaView. It covers building for
development, on both Unix-type systems (Linux, HP-UX, Solaris, macOS), and
Windows. Note that Unix-like environments such as Cygwin and MinGW are not
officially supported. However, patches to fix problems with these platforms
will be considered for inclusion.

ParaView depends on several open source tools and libraries such as Python, Qt,
CGNS, HDF5, etc. Some of these are included in the ParaView source itself
(e.g., HDF5), while others are expected to be present on the machine on which
ParaView is being built (e.g., Python, Qt).

## Obtaining the source

To obtain ParaView's sources locally, clone this repository using
[Git][git].

```sh
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
```

## Building

ParaView supports all of the common generators supported by CMake. The Ninja,
Makefiles, and Visual Studio generators are the most well-tested however.

### Prerequisites

ParaView only requires a few packages in order to build in general, however
specific features may require additional packages to be provided to ParaView's
build configuration.

Required:

  * [CMake][cmake]
    - Version 3.10 or newer, however, the latest version is always recommended
  * Supported compiler
    - GCC 4.8 or newer
    - Clang 4 or newer
    - Xcode 9 or newer
    - Visual Studio 2015 or newer

Optional dependencies:

  * [Python][python]
    - When using Python 2, at least 2.7 is required
    - When using Python 3, at least 3.3 is required
  * [Qt5][qt]
    - Version 5.9 or newer

#### Installing CMake

CMake is a tool that makes cross-platform building simple. On several systems
it will probably be already installed or available through system package
management utilities. If it is not, there are precompiled binaries available on
[CMake's download page][cmake-download].

#### Installing Qt

ParaView uses Qt as its GUI library. Precompiled binaries are available on
[Qt's website][qt-download].

Note that on Windows, the compiler used for building ParaView must match the
compiler version used to build Qt.

The Linux packages for Qt 5.9 use a version of protobuf that may conflict with
that used by ParaView. If, when running ParaView, error messages about a
mismatch in protobuf versions appears, moving the `libqgtk3.so` plugin out of
the `plugins/platformthemes` directory has been sufficient in the past.

### Optional Additions

#### Download And Install ffmpeg (`.avi`) movie libraries

When the ability to write `.avi` files is desired, and writing these files is
not supported by the OS, ParaView can use the ffmpeg library. This is generally
true for Linux. Source code for ffmpeg can be obtained from [the
website][ffmpeg].

#### MPI

To run ParaView in parallel, an [MPI][mpi] implementation is required. If an
MPI implementation that exploits special interconnect hardware is provided on
your system, we suggest using it for optimal performance. Otherwise, on
Linux/Mac, we suggest either [OpenMPI][openmpi] or [MPICH][mpich]. On Windows,
[Microsoft MPI][msmpi] is required.

#### Python

In order to use scripting, [Python][python] is required (versions 2.7 and 3.3).
Python is also required in order to build ParaViewWeb support.

#### OSMesa

Off-screen Mesa can be used as a software-renderer for running ParaView on a
server without hardware OpenGL acceleration. This is usually available in
system packages on Linux. For example, the `libosmesa6-dev` package on Debian
and Ubuntu. However, for older machines, building a newer version of Mesa is
likely necessary for bug fixes and support. Its source and build instructions
can be found on [its website][mesa].

## Creating the Build Environment

### Linux (Ubuntu/Debian)

  * `sudo apt install` the following packages:
    - `build-essential`
    - `cmake`
    - `mesa-common-dev`
    - `mesa-utils`
    - `freeglut3-dev`
    - `ninja-build`
      - `ninja` is a speedy replacement for `make`, highly recommended.

*Note*: If you are using an Ubuntu-provided compiler, there is a known issue
with the optional Python linking. This case is hard to auto-detect, so if
undefined symbol errors related to Python symbols arise, setting
`vtk_undefined_symbols_allowed=OFF` may resolve the errors. If it does not,
please file a new issue.

### Windows

  * [Visual Studio Community Edition][visual-studio]
  * Use "x64 Native Tools Command Prompt" for the installed Visual Studio
    version to configure with CMake and to build with ninja.
  * Get [ninja][ninja]. Unzip the binary and put it in `PATH`.

## Building

In order to build, CMake requires two steps, configure and build. ParaView
itself does not support what are known as in-source builds, so the first step
is to create a build directory.

!!! note
    On Windows, there have historically been issues with long paths to the
    build directory. These should have been addressed, but they may appear
    again. If any are seen, please report them to [the issue
    tracker][paraview-issues].

```sh
mkdir -p paraview/build
cd paraview/build
ccmake ../path/to/paraview/source # -GNinja may be added to use the Ninja generator
```

CMake's GUI has input entries for the build directory and the generator
already. Note that on Windows, the GUI must be launched from a "Native Tools
Command Prompt" available with Visual Studio in the start menu.

### Build Settings

ParaView has a number of settings available for its build. The common variables
to modify include:

  * `PARAVIEW_BUILD_SHARED_LIBS` (default `ON`): If set, shared libraries will
    be built. This is usually what is wanted.
  * `PARAVIEW_BUILD_QT_GUI` (default `ON`): Builds the `paraview` GUI
    application.
  * `PARAVIEW_USE_MPI` (default `OFF`): Whether MPI support will be available
    or not.
  * `PARAVIEW_USE_OSPRAY` (default `OFF`): Whether OSPRay ray-tracing support
    will be available or not.
  * `PARAVIEW_ENABLE_PYTHON` (default `OFF`): Whether Python
    support will be available or not.
  * `PARAVIEW_ENABLE_WEB` (default `OFF`; requires `PARAVIEW_ENABLE_PYTHON`):
    Whether ParaViewWeb support will be available or not.
  * `PARAVIEW_PLUGIN_ENABLE_<name>` (default varies): Whether to enable a
    plugin or not.

ParaView uses VTK's module system to control its build. This infrastructure
provides a number of variables to control modules which are not otherwise
controlled by the other options provided.

  * `VTK_MODULE_USE_EXTERNAL_<name>` (default depends on
    `PARAVIEW_USE_EXTERNAL`): Use an external source for the named third-party
    module rather than the copy contained within the ParaView source tree.
  * `VTK_MODULE_ENABLE_<name>` (default `DEFAULT`): Change the build settings
    for the named module. Valid values are those for the module system's build
    settings (see below).
  * `VTK_GROUP_ENABLE_<name>` (default `DEFAULT`): Change the default build
    settings for modules belonging to the named group. Valid values are those
    for the module system's build settings (see below).

For variables which use the module system's build settings, the valid values are as follows:

  * `YES`: Require the module to be built.
  * `WANT`: Build the module if possible.
  * `DEFAULT`: Use the settings by the module's groups and
    `PARAVIEW_BUILD_ALL_MODULES`.
  * `DONT_WANT`: Don't build the module unless required as a dependency.
  * `NO`: Do not build the module.

If any `YES` module requires a `NO` module, an error is raised.

Less common, but variables which may be of interest to some:

  * `PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION` (default `OFF`): If set, the HTML
    documentation for ParaView's C++, Python, and proxies will be generated.
  * `PARAVIEW_ENABLE_EXAMPLES` (default `OFF`): If set, ParaView's example code
    will be added as tests to the ParaView test suite.
  * `PARAVIEW_ENABLE_LOGGING` (default `ON`): If set, enhanced logging will be
    enabled.
  * `PARAVIEW_USE_VTKM` (default `ON`): Enable VTK-m accelerated algorithms in ParaView.
  * `PARAVIEW_ENABLE_VISITBRIDGE` (default `OFF`): Enable support for VisIt
    readers.
  * `PARAVIEW_BUILD_TESTING` (default `OFF`): Whether to build tests or not.
    Valid values are `OFF` (no testing), `WANT` (enable tests as possible), and
    `ON` (enable all tests; may error out if features otherwise disabled are
    required by test code).
  * `PARAVIEW_ENABLE_KITS` (default `OFF`; requires CMake 3.12+): Compile
    ParaView into a smaller set of libraries. Can be useful on platforms where
    ParaView takes a long time to launch due to expensive disk access.
  * `PARAVIEW_ENABLE_CATALYST` (default `ON`): Whether to build Catalyst
    CoProcessing support or not.

<!--
These variables should be documented once they're effective again.

  * `PARAVIEW_FREEZE_PYTHON` (default `OFF`): Whether Python modules will be
    frozen into ParaView itself or installed as a normal Python package.
-->

More advanced options:

  * `PARAVIEW_BUILD_ALL_MODULES` (default `OFF`): If set, ParaView will enable
    all modules not disabled by other features.
  * `PARAVIEW_ENABLE_NVPIPE` (default `OFF`): Use [nvpipe][nvpipe] image
    compression when communicating the GPU. Requires CUDA and an NVIDIA GPU.
  * `PARAVIEW_ENABLE_GDAL` (default `OFF`): Enable support for reading GDAL
    files.
  * `PARAVIEW_ENABLE_LAS` (default `OFF`): Enable support for reading LAS
    files.
  * `PARAVIEW_ENABLE_OPENTURNS` (default `OFF`): Enable support for reading
    OpenTURNS files.
  * `PARAVIEW_ENABLE_PDAL` (default `OFF`): Enable support for reading PDAL
    files.
  * `PARAVIEW_ENABLE_MOTIONFX` (default `OFF`): Enable support for reading
    MotionFX files.
  * `PARAVIEW_ENABLE_MOMENTINVARIANTS` (default `OFF`): Enable
    MomentInvariants filters.
  * `PARAVIEW_ENABLE_XDMF2` (default `OFF`): Enable support for reading Xdmf2
    files.
  * `PARAVIEW_ENABLE_XDMF3` (default `OFF`): Enable support for reading Xdmf3
    files.
  * `PARAVIEW_ENABLE_FFMPEG` (default `OFF`; not available on Windows): Enable
    FFmpeg support.
  * `PARAVIEW_ENABLE_COSMOTOOLS` (default `OFF`; requires `PARAVIEW_USE_MPI`
    and not available on Windows): Enable support for CosmoTools which includes
    GenericIO readers and writers as well as some point cloud algorithms.
  * `PARAVIEW_USE_MPI_SSEND` (default `OFF`; requires `PARAVIEW_USE_MPI`): Use
    synchronous send commands for communication.
  * `PARAVIEW_USE_ICE_T` (default `OFF`; requires `PARAVIEW_USE_MPI`): Use
    Ice-T for parallel rendering.
  * `PARAVIEW_INITIALIZE_MPI_ON_CLIENT` (default `ON`; requires
    `PARAVIEW_USE_MPI`): Initialize MPI on client processes by default.
  * `PARAVIEW_ENABLE_QT_SUPPORT` (default `OFF`; implied by
    `PARAVIEW_BUILD_QT_GUI`): Enable Qt support.
  * `PARAVIEW_USE_QTHELP` (default `ON`; requires
    `PARAVIEW_ENABLE_QT_SUPPORT`): Use Qt's help infrastructure for runtime
    documentation.
  * `PARAVIEW_ENABLE_COMMANDLINE_TOOLS` (default `ON`; implied by
    `PARAVIEW_BUILD_QT_GUI`): Build command line tools such as `pvserver` and
    `pvrenderserver`.
  * `PARAVIEW_USE_EXTERNAL` (default `OFF`): Whether to prefer external third
    party libraries or the versions ParaView's source contains.
  * `PARAVIEW_VERSIONED_INSTALL` (default `ON`): Whether to add version numbers
    to ParaView's include and plugin directories in the install tree.
  * `PARAVIEW_CUSTOM_LIBRARY_SUFFIX` (default depends on
    `PARAVIEW_VERSIONED_INSTALL`): The custom suffix for libraries built by
    ParaView. Defaults to either an empty string or `pvX.Y` where `X` and `Y`
    are ParaView's major and minor version components, respectively.
  * `PARAVIEW_PLUGINS_DEFAULT` (default `ON`): The state for ParaView's set of
    enabled-by-default plugins. Note that this variable only really has an
    effect on initial configures or newly added plugins.
  * `PARAVIEW_INSTALL_DEVELOPMENT_FILES` (default `ON`): If set, ParaView will
    install its headers, CMake API, etc. into its install tree for use.
  * `PARAVIEW_RELOCATABLE_INSTALL` (default `ON`): If set, the install tree
    will be relocatable to another path. If unset, the install tree may be tied
    to the build machine with absolute paths, but finding dependencies in
    non-standard locations may require work without passing extra information
    when consuming ParaView.

<!--
These variables should be documented once they're effective again.

  * `PARAVIEW_USE_EXTERNAL_VTK` (default `OFF`): Use an externally provided
    VTK. Note that ParaView has fairly narrow requirements for the VTK it can
    use, so only very recent versions are likely to work.
  * `PARAVIEW_BUILD_CATALYST_ADAPTORS` (default `OFF`; requires
    `PARAVIEW_ENABLE_CATALYST` and not available on Windows): If set,
    ParaView's example Catalyst adaptors will be added as tests to the ParaView
    test suite.
-->

## Building documentation

The following targets are used to build documentation for ParaView:

  * `ParaViewDoxygenDoc` - build the doxygen documentation from ParaView's C++ source files.
  * `ParaViewPythonDoc` - build the documentation from ParaView's Python source files.
  * `ParaViewDoc-TGZ` - build a gzipped tarball of ParaView documentation.

[cmake]: https://cmake.org
[cmake-download]: https://cmake.org/download
[ffmpeg]: https://ffmpeg.org
[git]: https://git-scm.org
[mesa]: https://www.mesa3d.org
[mpi]: https://www.mcs.anl.gov/research/projects/mpi
[ninja]: https://ninja-build.org
[msmpi]: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi
[mpich]: https://www.mpich.org
[nvpipe]: https://github.com/NVIDIA/NvPipe
[openmpi]: https://www.open-mpi.org
[paraview-issues]: https://gitlab.kitware.com/paraview/paraview/issues
[python]: https://python.org
[qt]: https://qt.io
[qt-download]: https://download.qt.io/official_releases/qt
[visual-studio]: https://visualstudio.microsoft.com/vs
