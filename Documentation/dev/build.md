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

The first chapter is a getting started guide by OS that is very helpful if you have never
built ParaView before and do not know which options you need.
If you are looking for the generic help, please read the [Complete Compilation Guide](#complete-compilation-guide)

## Getting Started Guide
This is a section intended to help those that have never built ParaView before, are not
experienced with compilation in general or have no idea which option they may need when building ParaView.
If you follow this guide, you will be able to compile and run a standard version of ParaView
for your operating system. It will be built with the python wrapping, MPI capabilities and multithread capabilities.

 * If you are using a Linux distribution, please see [the Linux part](#linux),
 * If you are using Microsoft Windows, please see [the Windows part](#windows),
 * If you are using another OS, feel free to provide compilation steps.

### Linux

#### Dependencies
Please run the command in a terminal to install the following dependencies depending of your linux distribution.

##### Ubuntu 18.04 LTS / Debian 10
`sudo apt-get install git cmake build-essential libgl1-mesa-dev libxt-dev qt5-default libqt5x11extras5-dev libqt5help5 qttools5-dev qtxmlpatterns5-dev-tools libqt5svg5-dev python3-dev python3-numpy libopenmpi-dev libtbb-dev ninja-build`

##### Centos 7

###### CMake
Download and install [cmake][cmake-download]) as the packaged version is not enough considering that
CMake 3.12 or higher is needed.

###### Others
`sudo yum install python3-devel openmpi-devel mesa-libGL-devel libX11-devel libXt-devel qt5-qtbase-devel qt5-qtx11extras-devel qt5-qttools-devel qt5-qtxmlpatterns-devel tbb-devel ninja-build git`

###### Environement
```sh
alias ninja=ninja-build
export PATH=$PATH:/usr/lib64/openmpi/bin/
```

##### ArchLinux
`sudo pacman -S base-devel ninja openmpi tbb qt python python-numpy cmake`

##### Other distribution
If you are using another distribution, please try to adapt the package list.
Feel free to then provide it so we can integrate it in this guide by creating an [issue][paraview-issues].

#### Build

To build ParaView developement version (usually refered as "master"), please run the following commands in a terminal :
```sh
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
mkdir paraview_build
cd paraview_build
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DCMAKE_BUILD_TYPE=Release ../paraview
ninja
```

To build a specific ParaView version, eg: v5.6.0 , please run the following commands in a terminal while replacing "tag" by the version you want to build
```sh
git clone https://gitlab.kitware.com/paraview/paraview.git
mkdir paraview_build
cd paraview
git checkout tag
git submodule update --init --recursive
cd ../paraview_build
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DCMAKE_BUILD_TYPE=Release ../paraview
ninja
```

#### Run
Double click on the paraview executable in the bin directory or run in the previous terminal

```sh
./bin/paraview
```

### Windows

Note: the following steps concerning Visual Studio 2015 can also be applied to Visual Studio 2019.
If so, beware to use the msvc2019_64 Qt Version and the Developer Command Prompt for VS 2019.

#### Dependencies
 * Download and install [git bash for windows][gitforwindows]
 * Download and install [cmake][cmake-download]
 * Download and install [Visual Studio 2015 Community Edition][visual-studio]
 * Download [ninja-build][ninja] and drop `ninja.exe` in `C:\Windows\`
 * Download and install both `msmpisetup.exe` and `msmpisdk.msi` from [Microsoft MPI][msmpi]
 * Download and install [Python for windows][pythonwindows], make sure to add the path to your Python installation folder to the `PATH` environnement variable.
 * Download and install [Qt 5.12.3][qt-download-5.12.3] for windows, make sure to check the MSVC 2015 64-bit component during installation, make sure to add `C:\Qt\Qt5.12.3\5.12.3\msvc2015_64\bin` to your `PATH` environnement variable.

#### Recover the source
 * Open git bash
 * To build ParaView developement version (usually refered as "master"), run the following commands:

```sh
cd C:
mkdir pv
cd pv
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
mv paraview pv
mkdir pvb
```

 * Or, to build a specific ParaView version, eg: v5.6.0 , please run the following commands while replacing "tag" by the version you want to build

```sh
cd C:
mkdir pv
cd pv
git clone https://gitlab.kitware.com/paraview/paraview.git
mv paraview pv
mkdir pvb
cd pv
git checkout tag
git submodule update --init --recursive
```

#### Build

 * Open VS2015 x64 Native Tools Command Prompt and run the following commands
```sh
cd C:\pv\pvb
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=OpenMP -DCMAKE_BUILD_TYPE=Release ..\pv
ninja
```

#### Run

 * Double click on the `C:\pv\pvb\bin\paraview` executable

## Complete Compilation Guide

### Obtaining the source

To obtain ParaView's sources locally, clone this repository using
[Git][git].

```sh
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
```

### Building

ParaView supports all of the common generators supported by CMake. The Ninja,
Makefiles, and Visual Studio generators are the most well-tested however.

#### Prerequisites

ParaView only requires a few packages in order to build in general, however
specific features may require additional packages to be provided to ParaView's
build configuration.

Required:

  * [CMake][cmake]
    - Version 3.12 or newer, however, the latest version is always recommended
  * Supported compiler
    - GCC 4.8 or newer
    - Clang 4 or newer
    - Xcode 9 or newer
    - Visual Studio 2015 or newer

Optional dependencies:

  * [Python][python]
    - At least 3.3 is required
  * [Qt5][qt]
    - Version 5.9 or newer

##### Installing CMake

CMake is a tool that makes cross-platform building simple. On several systems
it will probably be already installed or available through system package
management utilities. If it is not, there are precompiled binaries available on
[CMake's download page][cmake-download].

##### Installing Qt

ParaView uses Qt as its GUI library. Precompiled binaries are available on
[Qt's website][qt-download].

Note that on Windows, the compiler used for building ParaView must match the
compiler version used to build Qt.

The Linux packages for Qt 5.9 use a version of protobuf that may conflict with
that used by ParaView. If, when running ParaView, error messages about a
mismatch in protobuf versions appears, moving the `libqgtk3.so` plugin out of
the `plugins/platformthemes` directory has been sufficient in the past.

#### Optional Additions

##### Download And Install ffmpeg (`.avi`) movie libraries

When the ability to write `.avi` files is desired, and writing these files is
not supported by the OS, ParaView can use the ffmpeg library. This is generally
true for Linux. Source code for ffmpeg can be obtained from [the
website][ffmpeg].

##### MPI

To run ParaView in parallel, an [MPI][mpi] implementation is required. If an
MPI implementation that exploits special interconnect hardware is provided on
your system, we suggest using it for optimal performance. Otherwise, on
Linux/Mac, we suggest either [OpenMPI][openmpi] or [MPICH][mpich]. On Windows,
[Microsoft MPI][msmpi] is required.

##### Python

In order to use scripting, [Python][python] is required (version 3.3). Python
is also required in order to build ParaViewWeb support.

##### OSMesa

Off-screen Mesa can be used as a software-renderer for running ParaView on a
server without hardware OpenGL acceleration. This is usually available in
system packages on Linux. For example, the `libosmesa6-dev` package on Debian
and Ubuntu. However, for older machines, building a newer version of Mesa is
likely necessary for bug fixes and support. Its source and build instructions
can be found on [its website][mesa].

### Creating the Build Environment

#### Linux (Ubuntu/Debian)

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

#### Windows

  * [Visual Studio 2015 Community Edition][visual-studio]
  * Use "x64 Native Tools Command Prompt" for the installed Visual Studio
    version to configure with CMake and to build with ninja.
  * Get [ninja][ninja]. Unzip the binary and put it in `PATH`.

### Building

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

#### Build Settings

ParaView has a number of settings available for its build. These are categorized
as build options, capability option, feature options and miscellanoues options.


#### Build Options

These options impact the build. These begin with the prefix `PARAVIEW_BUILD_`.
The common variables to modify include:

  * `PARAVIEW_BUILD_SHARED_LIBS` (default `ON`): If set, shared libraries will
    be built. This is usually what is wanted.

Less common, but variables which may be of interest to some:

  * `PARAVIEW_BUILD_EDITION` (default `CANONICAL`): Choose which features to
    enable in this build. This is useful to generate ParaView builds with
    limited features. More on this later.
  * `PARAVIEW_BUILD_EXAMPLES` (default `OFF`): If set, ParaView's example code
    will be added as tests to the ParaView test suite.
  * `PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION` (default `OFF`): If set, the HTML
    documentation for ParaView's C++, Python, and proxies will be generated.
  * `PARAVIEW_BUILD_TESTING` (default `OFF`): Whether to build tests or not.
    Valid values are `OFF` (no testing), `WANT` (enable tests as possible), and
    `ON` (enable all tests; may error out if features otherwise disabled are
    required by test code).
  * `PARAVIEW_BUILD_VTK_TESTING` (default `OFF`): Whether to build tests for the
    VTK codebase built by ParaView. Valid values are same as
    `PARAVIEW_BUILD_TESTING`.

More advanced build options are:

  * `PARAVIEW_BUILD_ALL_MODULES` (default `OFF`): If set, ParaView will enable
    all modules not disabled by other features.
  * `PARAVIEW_BUILD_LEGACY_REMOVE` (default `OFF`): Remove legacy / deprecated
    code.
  * `PARAVIEW_BUILD_LEGACY_SILENT` (default `OFF`): Silence all legacy
    / deprecated code messages.
  * `PARAVIEW_BUILD_WITH_EXTERNAL` (default `OFF`): When set to `ON`, the build
    will try to use external copies of all included third party libraries unless
    explicitly overridden.
  * `PARAVIEW_BUILD_WITH_KITS` (default `OFF`): Compile ParaView into a smaller
    set of libraries. Can be useful on platforms where ParaView takes a long
    time to launch due to expensive disk access.
  * `PARAVIEW_BUILD_ID` (default `""`): A build ID for the ParaView build. It
    can be any arbitrary value which can be used to indicate the provenance of
    ParaView.

#### Capability settings

These settings control capabitities of the build. These begin with the prefix
`PARAVIEW_USE_`. The common variables to modify include:

  * `PARAVIEW_USE_QT` (default `ON`): Builds the `paraview` GUI application.
  * `PARAVIEW_USE_MPI` (default `OFF`): Whether MPI support will be available
    or not.
  * `PARAVIEW_USE_PYTHON` (default `OFF`): Whether Python
    support will be available or not.

Less common, but potentially useful variables are:

  * `PARAVIEW_USE_VTKM` (default `ON`): Whether VTK-m based filters are enabled.
  * `PARAVIEW_USE_FORTRAN` (default `ON` if Fortran compiler found): Enable
     Fortran support for Catalyst libraries.

#### Feature settings

These settings control optional features. These begin with the prefix
`PARAVIEW_ENABLE_`. The common variables to modify include:

  * `PARAVIEW_ENABLE_RAYTRACING` (default `OFF`): Enable ray-tracing support
    with OSPray and/or OptiX. Requires appropriate external libraries.
  * `PARAVIEW_ENABLE_WEB` (default `OFF`; requires `PARAVIEW_USE_PYTHON`):
    Whether ParaViewWeb support will be available or not.

More advanced / less common options include:

  * `PARAVIEW_ENABLE_VISITBRIDGE` (default `OFF`): Enable support for VisIt
    readers.
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
  * `PARAVIEW_ENABLE_LOOKINGGLASS` (default `OFF`): Enable LookingGlass display.
  * `PARAVIEW_ENABLE_XDMF2` (default `OFF`): Enable support for reading Xdmf2
    files.
  * `PARAVIEW_ENABLE_XDMF3` (default `OFF`): Enable support for reading Xdmf3
    files.
  * `PARAVIEW_ENABLE_FFMPEG` (default `OFF`; not available on Windows): Enable
    FFmpeg support.
  * `PARAVIEW_ENABLE_COSMOTOOLS` (default `OFF`; requires `PARAVIEW_USE_MPI`
    and not available on Windows): Enable support for CosmoTools which includes
    GenericIO readers and writers as well as some point cloud algorithms.


#### Plugin settings

ParaView build includes several plugins. These can be enabled / disabled using the
following options:

  * `PARAVIEW_PLUGINS_DEFAULT` (default `ON`): Pass this flag to the command
    line using `-DPARAVIEW_PLUGINS_DEFAULT=OFF` before the first cmake run to
    disable all plugins by default. Note this has no impact after the first
    cmake configure and hence must be passed on the command line itself.
  * `PARAVIEW_PLUGIN_ENABLE_<name>` (default varies): Whether to enable a
    plugin or not.
  * `PARAVIEW_PLUGIN_AUTOLOAD_<name>` (default `OFF`): Whether to autoload a
    plugin at startup or not. Note that this affects all clients linking to
    ParaView's plugin target.

#### Miscellaneous settings
ParaView uses VTK's module system to control its build. This infrastructure
provides a number of variables to control modules which are not otherwise
controlled by the other options provided.

  * `VTK_MODULE_USE_EXTERNAL_<name>` (default depends on
    `PARAVIEW_BUILD_WITH_EXTERNAL`): Use an external source for the named third-party
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

More advanced options:

  * `PARAVIEW_INITIALIZE_MPI_ON_CLIENT` (default `ON`; requires
    `PARAVIEW_USE_MPI`): Initialize MPI on client processes by default.
  * `PARAVIEW_USE_QTHELP` (default `ON`; requires
    `PARAVIEW_USE_QT`): Use Qt's help infrastructure for runtime
    documentation.
  * `PARAVIEW_VERSIONED_INSTALL` (default `ON`): Whether to add version numbers
    to ParaView's include and plugin directories in the install tree.
  * `PARAVIEW_CUSTOM_LIBRARY_SUFFIX` (default depends on
    `PARAVIEW_VERSIONED_INSTALL`): The custom suffix for libraries built by
    ParaView. Defaults to either an empty string or `pvX.Y` where `X` and `Y`
    are ParaView's major and minor version components, respectively.
  * `PARAVIEW_INSTALL_DEVELOPMENT_FILES` (default `ON`): If set, ParaView will
    install its headers, CMake API, etc. into its install tree for use.
  * `PARAVIEW_RELOCATABLE_INSTALL` (default `ON`): If set, the install tree
    will be relocatable to another path or machine. External dependencies
    needed by ParaView which are in non-standard locations may need manual
    settings in ParaView-using projects (those which share an install prefix
    with ParaView should be OK though). If unset, the install tree will include
    hints for the location of its dependencies which may include
    build-machine-specific paths in the install tree.
  * `PARAVIEW_SERIAL_TESTS_USE_MPIEXEC` (default `OFF`): Used on HPC to run
    serial tests on compute nodes. If set, it prefixes serial tests with
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "1" ${MPIEXEC_PREFLAGS}

<!--
These variables should be documented once they're effective again.

  * `PARAVIEW_USE_EXTERNAL_VTK` (default `OFF`): Use an externally provided
    VTK. Note that ParaView has fairly narrow requirements for the VTK it can
    use, so only very recent versions are likely to work.
-->
## Building editions

A typical ParaView build includes several modules and dependencies. While these
are necessary for a fully functional application, there are cases (e.g. in situ
use-cases) where a build with limited set of features is adequate. ParaView build supports
this using the `PARAVIEW_BUILD_EDITION` setting. Supported values for this setting are:

* `CORE`: Build modules necessary for core ParaView functionality.
  This does not include rendering.
* `RENDERING`: Build modules necessary for supporting rendering including views
  and representations. This includes everything in `CORE`.
* `CATALYST`: Build all modules necessary for in situ use cases without
  rendering and optional components like NetCDF- and HDF5-based readers and
  writers.
* `CATALYST_RENDERING`: Same as `CATALYST` but with rendering supported added.
* `CANONICAL` (default): Build modules necessary for standard ParaView build.


### Building documentation

The following targets are used to build documentation for ParaView:

  * `ParaViewDoxygenDoc` - build the doxygen documentation from ParaView's C++ source files.
  * `ParaViewPythonDoc` - build the documentation from ParaView's Python source files.
  * `ParaViewDoc-TGZ` - build a gzipped tarball of ParaView documentation.

## Using spack

[Spack][spack] is a package manager for supercomputers, Linux and macOS. ParaView is one of
the packages available in Spack. To install ParaView from spack, you can use:

```
spack install paraview
```

Please refer to [Spack documentation][spack-docs] for ways of customizing the install,
including choosing the version and/or variant to build. Based on the version chosen,
spack will download appropriate ParaView source and build it.

To make it easier to build ParaView using spack from an existing source checkout, we have included
relevant spack `package.yaml` files within the ParaView codebase itself. This
also makes it easier to keep the spack package up-to-date with any changes to
the ParaView buildsystem. With every release (and as frequently as required), we
will push the changes to the ParaView paraview.yaml file upstream to the
official spack repository.

To build your existing source checkout of ParaView using Spack, here are the
steps:

```bash
# assuming you've installed spack as documented in spack docs
# and activate the spack environment appropriately

# add custom paraview/package.yaml
> spack repo add $PARAVIEW_SOURCE_DIR/Utilities/spack/repo

# use info to confirm that the paraview package is available
# only one version should be available
> spack info paraview

# install similar to any other spack package
# e.g. following command installs osmesa-capable ParaView
# with mpich

> spack install paraview+osmesa^mesa~glx^mpich
```

[cmake-download]: https://cmake.org/download
[cmake]: https://cmake.org
[ffmpeg]: https://ffmpeg.org
[git]: https://git-scm.org
[gitforwindows]: https://gitforwindows.org/
[mesa]: https://www.mesa3d.org
[mpi]: https://www.mcs.anl.gov/research/projects/mpi
[mpich]: https://www.mpich.org
[msmpi]: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi
[ninja]: https://github.com/ninja-build/ninja/releases
[nvpipe]: https://github.com/NVIDIA/NvPipe
[openmpi]: https://www.open-mpi.org
[paraview-issues]: https://gitlab.kitware.com/paraview/paraview/-/issues
[python]: https://python.org
[pythonwindows]: https://www.python.org/downloads/windows/
[qt-download]: https://download.qt.io/official_releases/qt
[qt]: https://qt.io
[qt-download-5.12.3]: https://download.qt.io/archive/qt/5.12/5.12.3/
[tbb]: https://github.com/intel/tbb/releases
[visual-studio]: https://visualstudio.microsoft.com/vs/older-downloads
[spack]: https://spack.io/
[spack-docs]: https://spack.readthedocs.io/en/latest/
