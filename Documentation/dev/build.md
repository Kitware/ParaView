# Building ParaView

This page describes how to build and install ParaView. It covers building for
development, on both Linux and Windows. Please Note that Linux (x86_64), Windows (x86_64)
and macOS (x86_64 and arm64) version are built and tested by our continuous
integration system and are considered supported environments.

Any other environnements and architecture (including Cygwin, MingGW, PowerPC) are considered
non-officially supported, however, patches to fix problems with these platforms will
be considered for inclusion.

ParaView depends on several open source tools and libraries such as Python, Qt,
CGNS, HDF5, etc. Some of these are included in the ParaView source itself
(e.g., HDF5), while others are expected to be present on the machine on which
ParaView is being built (e.g., Python, Qt).

The first section is a getting started guide by OS that is very helpful if you have never
built ParaView before and do not know which options you need.
If you are looking for the generic help, please read the [Complete Compilation Guide](#complete-compilation-guide)

## Getting Started Guide
This is a section intended to help those that have never built ParaView before, are not
experienced with compilation in general or have no idea which option they may need when building ParaView.
If you follow this guide, you will be able to compile and run a standard version of ParaView
for your operating system. It will be built with the Python wrapping, MPI capabilities and multithreading capabilities.

 * If you are using a Linux distribution, please see [the Linux part](#linux),
 * If you are using Microsoft Windows, please see [the Windows part](#windows),
 * If you are using macOS, please see [the macOS part](#macos),
 * If you are using another OS, feel free to provide compilation steps.

### Linux

#### Dependencies
Please run the command in a terminal to install the following dependencies depending of your linux distribution.

##### Ubuntu 22.04 LTS / Debian 12

`sudo apt-get install git cmake build-essential libgl1-mesa-dev libxt-dev libqt5x11extras5-dev libqt5help5 qttools5-dev qtxmlpatterns5-dev-tools libqt5svg5-dev python3-dev python3-numpy libopenmpi-dev libtbb-dev ninja-build qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools`

##### Ubuntu 18.04 LTS / Debian 10
`sudo apt-get install git cmake build-essential libgl1-mesa-dev libxt-dev qt5-default libqt5x11extras5-dev libqt5help5 qttools5-dev qtxmlpatterns5-dev-tools libqt5svg5-dev python3-dev python3-numpy libopenmpi-dev libtbb-dev ninja-build`

##### Centos 7

###### CMake
Download and install [cmake][cmake-download]) as the packaged version is not enough considering that
CMake 3.12 or higher is needed.

###### Others
`sudo yum install python3-devel openmpi-devel mesa-libGL-devel libX11-devel libXt-devel qt5-qtbase-devel qt5-qtx11extras-devel qt5-qttools-devel qt5-qtxmlpatterns-devel tbb-devel ninja-build git`

###### Environment
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

To build ParaView development version (usually refered as "master"), please run the following commands in a terminal:
```sh
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
mkdir paraview_build
cd paraview_build
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DCMAKE_BUILD_TYPE=Release ../paraview
ninja
```

To build a specific ParaView version, eg: v5.9.1, please run the following commands in a terminal while replacing "tag" by the version you want to build
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
Double click on the paraview executable in the `/bin` directory or run in the previous terminal

```sh
./bin/paraview
```

### macOS
These instructions have worked on a mid 2023 MacMini with an M2 chipset on macOS Ventura.

#### Install Homebrew
Please run the command in a terminal to install the following dependencies on your Mac via [Homebrew](https://brew.sh/) and add the relevant environment variables for brew.
```zsh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
(echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
```
##### Install dependencies (largely following the Ubuntu steps for Linux)
`brew install open-mpi cmake mesa tbb ninja gdal qt5`

##### Set build environment
```sh
echo 'export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"' >> ~/.zshrc
echo 'export LDFLAGS="-L/opt/homebrew/opt/qt@5/lib"' >> ~/.zshrc
echo 'export CPPFLAGS="-I/opt/homebrew/opt/qt@5/include"' >> ~/.zshrc
source ~/.zshrc
```

#### Build

To build a specific ParaView version, eg: v5.11.1, please run the following commands in a terminal
```sh
git clone https://gitlab.kitware.com/paraview/paraview.git
mkdir paraview_build
cd paraview
git checkout v5.11.1
git submodule update --init --recursive
cd ../paraview_build
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=TBB -DCMAKE_BUILD_TYPE=Release -DPARAVIEW_ENABLE_GDAL=ON ../paraview
ninja
```

#### Run
Double click on the paraview executable in the `/bin` directory or run in the previous terminal

```sh
./bin/paraview.app/Contents/MacOS/paraview
```

### Windows

Note: The following steps concerning Visual Studio 2019 can also be applied to newer versions.
If so, be sure to use the respective Qt Version (e.g. for VS 2022, use msvc2022_64) and the Native Tools Command Prompt.

#### Dependencies
 * Download and install [git bash for windows][gitforwindows]
 * Download and install [cmake][cmake-download]
 * Download and install [Visual Studio 2019 Community Edition][visual-studio]
 * Download [ninja-build][ninja] and drop `ninja.exe` in `C:\Windows\`
 * Download and install both `msmpisetup.exe` and `msmpisdk.msi` from [Microsoft MPI][msmpi]
 * Download and install [Python for Windows][pythonwindows], make sure to add the path to your Python installation folder to the `PATH` environnement variable.
 * Download and install [Qt 5.15.3][qt-download-5.15.3] for Windows, make sure to check the MSVC 2019 64-bit component during installation.
    * Make sure to add `C:\Qt\Qt5.15.3\5.15.3\msvc2019_64\bin` to your `PATH` environment variable.
    * You may also need to add an environment variable `QT_QPA_PLATFORM_PLUGIN_PATH`: `C:\Qt\Qt5.15.3\5.15.3\msvc2019_64\plugins\platforms`.

#### Recover the source
 * Open git bash
 * To build ParaView development version (usually referred to as `master`), run the following commands:

```sh
cd C:
mkdir pv
cd pv
git clone --recursive https://gitlab.kitware.com/paraview/paraview.git
mv paraview pv
mkdir pvb
```

 * Or, to build a specific ParaView version, eg: v5.9.1, please run the following commands while replacing "tag" by the version you want to build

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

 * Open VS2019 x64 Native Tools Command Prompt and run the following commands
```sh
cd C:\pv\pvb
cmake -GNinja -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_SMP_IMPLEMENTATION_TYPE=STDThread -DCMAKE_BUILD_TYPE=Release ..\pv
ninja
```

Note: If you want to build ParaView with `CMAKE_BUILD_TYPE=Debug` you also need to add the option `-DPARAVIEW_WINDOWS_PYTHON_DEBUGGABLE=ON`.

#### Run

 * Double click on the `C:\pv\pvb\bin\paraview.exe` executable

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

ParaView only requires a few packages to build with its basic capabilities. However,
specific features may require additional packages to be provided to ParaView's
build configuration.

Required:

  * [CMake][cmake]
    - Version 3.12 or newer, however, the latest version is always recommended
  * Supported compiler
    - GCC 8.0 or newer
    - Intel 19 or newer
    - IBM XL 17.1 or newer
    - Clang 5 or newer
    - Xcode 10 or newer
    - Visual Studio 2019 or newer

Optional dependencies:

  * [Python][python]
    - At least 3.3 is required
  * [Qt5][qt]
    - Version 5.12 or newer. Qt6 support is experimental and not tested yet.

##### Installing CMake

CMake is a tool that makes cross-platform building simple. On several systems
it will probably be already installed or available through system package
management utilities. If it is not, there are precompiled binaries available on
[CMake's download page][cmake-download].

##### Installing Qt

ParaView uses Qt to provide its graphical user interface. Precompiled binaries are available on
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
true for Linux. Source code for ffmpeg can be obtained from [its
website][ffmpeg].

##### MPI

To run ParaView in parallel, an [MPI][mpi] implementation is required. If an
MPI implementation that exploits special interconnect hardware is provided on
your system, we suggest using it for optimal performance. Otherwise, on
Linux/Mac, we suggest either [OpenMPI][openmpi] or [MPICH][mpich]. On Windows,
[Microsoft MPI][msmpi] is required.

##### Python

In order to use Python scripting, [Python][python] is required (version 3.3 or later). Python
is also required in order to build ParaViewWeb support.

##### OSMesa

Off-screen Mesa can be used as a software-renderer for running ParaView on a
server without hardware OpenGL acceleration. This is usually available in
system packages on Linux. For example, the `libosmesa6-dev` package on Debian
and Ubuntu. However, for older machines, building a newer version of Mesa is
likely necessary for bug fixes and support for features needed by ParaView.
Its source and build instructions can be found on [its website][mesa].

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
please file a new [issue][paraview-issues].

#### Windows

  * [Visual Studio 2019 Community Edition][visual-studio]
  * Use "x64 Native Tools Command Prompt" for the installed Visual Studio
    version to configure with CMake and to build with ninja.
  * Get [ninja][ninja]. Unzip the binary and put it in `PATH`.

### Building

In order to build, CMake requires two steps, configure and build. ParaView
itself does not support what are known as in-source builds, so the first step
is to create a build directory.

!!! note
    On Windows, there have historically been issues if the path to the
    build directory is too long. These issues should have been addressed in
    more recent versions of ParaView's build system, but they may appear again. If you
    see errors related to header files not being found at paths on your file
    system that do exist, please report them to [the issue
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
as build options, capability options, feature options and miscellaneous options.

#### Build Options

Options that impact the build begin with the prefix `PARAVIEW_BUILD_`.
Common variables to modify include:

  * `PARAVIEW_BUILD_SHARED_LIBS` (default `ON`): If set, shared libraries will
    be built. This is usually what is wanted.

Less common, but variables which may be of interest to some:

  * `PARAVIEW_BUILD_EDITION` (default `CANONICAL`): Choose which features to
    enable in this build. This is useful to generate ParaView builds with
    limited features. More on this later.
  * `PARAVIEW_ENABLE_EXAMPLES` (default `OFF`): If set, ParaView's example code
    will be added as tests to the ParaView test suite. These tests may be built
    and run using the `paraview-examples` target.
  * `PARAVIEW_BUILD_DEVELOPER_DOCUMENTATION` (default `OFF`): If set, the HTML
    documentation for ParaView's C++, Python, and proxies will be generated.
  * `PARAVIEW_PLUGIN_DISABLE_XML_DOCUMENTATION` (default `OFF`): Whether
    plugin XML documentation is forcefully disabled.
  * `PARAVIEW_BUILD_TESTING` (default `OFF`): Whether to build tests or not.
    Valid values are `OFF` (no testing), `DEFAULT` (enable tests which have all
    test dependencies satisfied), `WANT` (enable test dependencies as possible;
    see vtk/vtk#17509), and `ON` (enable all tests; may error out if features
    otherwise disabled are required by test code).
  * `PARAVIEW_BUILD_VTK_TESTING` (default `OFF`): Whether to build tests for the
    VTK codebase built by ParaView. Valid values are same as
    `PARAVIEW_BUILD_TESTING`.
  * `PARAVIEW_ENABLE_CATALYST` (default `OFF`): Whether to build the ParaView
    implementation of Catalyst.

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
  * `PARAVIEW_BUILD_WITH_KITS` (default `OFF`; requires
    `PARAVIEW_BUILD_SHARED_LIBS`): Compile ParaView into a smaller set of
    libraries. Can be useful on platforms where ParaView takes a long time to
    launch due to expensive disk access.
  * `PARAVIEW_BUILD_ID` (default `""`): A build ID for the ParaView build. It
    can be any arbitrary value which can be used to indicate the provenance of
    ParaView.
  * `PARAVIEW_GENERATE_SPDX` (default `OFF`): When compiling ParaView, generate
    a SPDX file for each of ParaView modules containing license and copyright
    information.

#### Capability settings

These settings control capabitities of the build. These begin with the prefix
`PARAVIEW_USE_`. The common variables to modify include:

  * `PARAVIEW_USE_QT` (default `ON`): Builds the `paraview` GUI application.
  * `PARAVIEW_USE_MPI` (default `OFF`): Whether MPI support will be available
    or not.
  * `PARAVIEW_USE_PYTHON` (default `OFF`): Whether Python
    support will be available or not.

Less common, but potentially useful variables are:

  * `PARAVIEW_USE_VISKORES` (default `ON`): Whether Viskores based filters are enabled.
  * `PARAVIEW_USE_FORTRAN` (default `ON` if Fortran compiler found): Enable
     Fortran support for Catalyst libraries.
  * `PARAVIEW_USE_CUDA` (default `OFF`): Enable CUDA support in ParaView.
  * `PARAVIEW_USE_HIP` (default `OFF`, requires CMake >= 3.21 and NOT
    `PARAVIEW_USE_CUDA`): Enable HIP support in ParaView.
  * `PARAVIEW_LOGGING_TIME_PRECISION` (default `3`): Change the precision of
    times output. Possible values are 3 for ms, 6 for us, 9 for ns.
  * `PARAVIEW_USE_SERIALIZATION` (default `OFF`): Whether VTK serialization is enabled.

#### Feature settings

These settings control optional features. These begin with the prefix
`PARAVIEW_ENABLE_`. The common variables to modify include:

  * `PARAVIEW_ENABLE_RAYTRACING` (default `OFF`): Enable ray-tracing support
    with OSPray and/or OptiX. Requires appropriate external libraries.
  * `PARAVIEW_ENABLE_WEB` (default `OFF`; requires `PARAVIEW_USE_PYTHON`):
    Whether ParaViewWeb support will be available or not.

These settings are used for translating purpose:
  * `PARAVIEW_BUILD_TRANSLATIONS` (default `OFF`): Enable translation
    files update and generation.
  * `PARAVIEW_TRANSLATIONS_DIRECTORY` (default `${CMAKE_BINARY_DIR}/Translations`):
    Path where the translation files will be generated on build.

More advanced / less common options include:

  * `PARAVIEW_ENABLE_VISITBRIDGE` (default `OFF`): Enable support for VisIt
    readers.
  * `PARAVIEW_ENABLE_NVPIPE` (default `OFF`): Use [nvpipe][nvpipe] image
    compression that uses the GPU when communicating. Requires CUDA and an NVIDIA GPU.
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
  * `PARAVIEW_ENABLE_LOOKINGGLASS` (default `OFF`): Enable support for LookingGlass displays.
  * `PARAVIEW_ENABLE_XDMF2` (default `OFF`): Enable support for reading Xdmf2
    files.
  * `PARAVIEW_ENABLE_XDMF3` (default `OFF`): Enable support for reading Xdmf3
    files.
  * `PARAVIEW_ENABLE_FFMPEG` (default `OFF`; not available on Windows): Enable
    FFmpeg support.
  * `PARAVIEW_ENABLE_COSMOTOOLS` (default `OFF`; requires `PARAVIEW_USE_MPI`
    and not available on Windows): Enable support for CosmoTools which includes
    GenericIO readers and writers as well as some point cloud algorithms.
  * `PARAVIEW_ENABLE_CGNS_READER` (default `ON` for CANONICAL builds, `OFF` for
    non-CANONICAL builds): Enable support for reading CGNS files. When building
    ParaView for e.g. CATALYST, this option allows building support for reading
    CGNS files. It will also build CGNSReader dependencies: HDF5 and CGNS.
  * `PARAVIEW_ENABLE_CGNS_WRITER` (default `ON` for CANONICAL builds, `OFF` for
    non-CANONICAL builds): Enable support for writing CGNS files. When building
    ParaView for e.g. CATALYST, this option allows building support for writing
    CGNS files. It will also build CGNSReader dependencies: HDF5 and CGNS. If
    `PARAVIEW_ENABLE_MPI` is `ON`, the parallel CGNS writer will also be built.
  * `PARAVIEW_ENABLE_OCCT` (default `OFF`): Enable support for reading OpenCascade
     file formats such as STEP and IGES.

#### Plugin settings

ParaView includes several optional plugins that can be enabled and disabled using the
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

For variables that use the module system's build settings, the valid values are as follows:

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
  * `PARAVIEW_USE_QTHELP` (default `ON`; requires `PARAVIEW_USE_QT`): Use Qt's
    help infrastructure for runtime documentation.
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
    "${MPIEXEC_EXECUTABLE}" "${MPIEXEC_NUMPROC_FLAG}" "1" ${MPI_PREFLAGS}
  * `PARAVIEW_SKIP_CLANG_TIDY_FOR_VTK` (defaults `ON`; requires
    `CMAKE_<LANG>_CLANG_TIDY`): If set, any `clang-tidy` settings will be
    cleared for the internal VTK build.
  * `PARAVIEW_TEST_DIR`: Used on HPC to set the test directory (default is
    "${CMAKE_BINARY_DIR}/Testing/Temporary") to a location that is writable from
    the compute nodes. Typically the user home directory is not.
  * `PARAVIEW_LINKER_FATAL_WARNINGS`: Specify if linker warnings must
    be considered as errors.
  * `PARAVIEW_EXTRA_COMPILER_WARNINGS`: Add compiler flags to do
    stricter checking when building.
  * `PARAVIEW_ENABLE_EXTRA_BUILD_WARNINGS` (default `OFF`; requires CMake >= 3.19):
    If set, PARAVIEW will enable additional build warnings.

<!--
These variables should be documented once they're effective again. Note that
various settings have a dependency on this that is not mentioned since the
option doesn't "exist" yet.

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

## Debugging facilities

ParaView's build is fairly complicated, so a few debugging facilities are
provided.

### General CMake

CMake provides the `--trace-expand` flag which causes CMake to log all commands
that it executes with variables expanded. This can help to trace logic and data
through the configure step.

Debugging `Find` modules can be done using the `--debug-find` flag (introduced
in CMake 3.17) to determine what CMake's `find_` commands are doing.

### VTK Modules

VTK's module system debugging facilities may be controlled by using the
following flags:

  * `ParaView_DEBUG_MODULE` (default `OFF`): If enabled, debugging is enabled.
    Specific portions of the module system may be debugged using the other
    flags.
  * `ParaView_DEBUG_MODULE_ALL` (default `OFF`): Enable all debugging messages.
  * `ParaView_DEBUG_MODULE_building` (default `OFF`): Log when modules are
    being built.
  * `ParaView_DEBUG_MODULE_enable` (default `OFF`): Log why modules are
    enabled.
  * `ParaView_DEBUG_MODULE_kit` (default `OFF`): Log information about
    discovered kits.
  * `ParaView_DEBUG_MODULE_module` (default `OFF`): Log information about
    discovered modules.
  * `ParaView_DEBUG_MODULE_provide` (default `OFF`): Log why a module is being
    built or not.
  * `ParaView_DEBUG_MODULE_testing` (default `OFF`): Log testing for VTK
    modules.

### ParaView Plugins

ParaView's plugin system has a similar setup:

  * `ParaView_DEBUG_PLUGINS` (default `OFF`): If enabled, debugging is enabled.
    Specific portions of the plugin system may be debugged using the other
    flags.
  * `ParaView_DEBUG_PLUGINS_ALL` (default `OFF`): Enable all debugging messages.
  * `ParaView_DEBUG_PLUGINS_building` (default `OFF`): Log when plugins are
    being built.
  * `ParaView_DEBUG_PLUGINS_plugin` (default `OFF`): Log information about
    discovered plugins.

### Building Python

The following target is provided to ensure that everything needed under `import
paraview` is available:

  * `paraview_all_python_modules`

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
[qt-download-5.15.3]: https://download.qt.io/archive/qt/5.15/5.15.3/
[tbb]: https://github.com/intel/tbb/releases
[visual-studio]: https://visualstudio.microsoft.com/vs/older-downloads
[spack]: https://spack.io/
[spack-docs]: https://spack.readthedocs.io/en/latest/
