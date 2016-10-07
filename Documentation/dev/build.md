Building ParaView
=================

This page describes how to build and install ParaView. It covers building for development, on both Unix-type systems (Linux, HP-UX, Solaris, Mac), and Windows.

ParaView depends on several open source tools and libraries such as Python, Qt, CGNS, HDF5, etc. Some of these are included in the ParaView source itself (e.g. HDF5), while others are expected to be present on the machine on which ParaView is being built (e.g. Python, Qt, CGNS).

Adapted from the [Paraview wiki](http://www.paraview.org/Wiki/ParaView:Build_And_Install) which has more complete but dated instructions.

Prerequisites
=============
* The ParaView build process requires [CMake](http://www.cmake.org/) version 2.8.8 or higher and a working compiler. On Unix-like operating systems, it also requires Make, while on Windows it requires Visual Studio and Ninja build.
* Building ParaView's user interface requires [Qt](http://www.qt.io/download-open-source/), version 4.7+ (4.8.\* is recommended, 5.7.\* also works). To compile ParaView, either the LGPL or commercial versions of Qt may be used. Also note that on Windows you need to choose a Visual Studio version to match binaries available for your Qt version.
* For Windows builds, unix-like environments such as Cygwin, or MinGW are not supported.

Download And Install CMake
--------------------------
CMake is a tool that makes cross-platform building simple. On several systems it will probably be already installed. If it is not, please use the following instructions to install it.

There are several precompiled binaries available at the [CMake download page](https://cmake.org/download/).

Add CMake to your PATH environment variable if you downloaded an archive and not an installer.

Download And Install Qt
--------------------------
ParaView uses Qt as its GUI library. Qt is required whenever the ParaView client is built with a GUI.

* [Download a release](http://download.qt.io/official_releases/qt/).
    - For binaries, use the latest stable version of qt-PLATFORM-opensource-VERSION.[tar.gz or zip or dmg or exe]. If this gives you trouble, version 4.8.2 is known to work. When downloading binaries, ensure that your compiler version matches the Qt compiler indicated. Verion 5.6+ supports Visual Studio 2015.
    - For source code, use the latest stable version of qt-everywhere-opensource-src-VERSION.[tar.gz or zip or dmg]. If this gives you trouble, version 4.8.2 is known to work.
* Developers have reported some issues with QT 5 on Mac and linux.

Compiler and Build Tool
-----------------------
Linux Ubuntu/Debian (16.04):

* sudo apt install
    - build-essential cmake git python-dev mesa-common-dev mesa-utils freeglut3-dev libhdf5-serial-dev autoconf libtool bison flex
* sudo python get-pip.py
    - sudo -H pip install numpy
    - sudo -H pip install Mako
* sudo apt-get install ninja-build
    - ninja is a speedy replacement for make, highly recommended.

Windows:

* Visual Studio 2015 Community Edition
* Use "VS2015 x64 Native Tools Command Prompt" for cmake and build.
* Get [Ninja Build](https://ninja-build.org/). Unzip the binary and put it on your path.

Optional Additions
------------------

### Download And Install ffmpeg (.avi) movie libraries

When the ability to write .avi files is desired, and writing these files is not supported by the OS, ParaView can attach to an ffmpeg library. This is generally true for Linux. Ffmpeg library source code is found here: [6](http://www.ffmpeg.org/)

### MPI
In order to run ParaView in parallel, MPI [1](http://www-unix.mcs.anl.gov/mpi/), [2](http://www.lam-mpi.org/) is also required.

### Python
In order to use scripting, [Python](http://www.python.org/) is required (version 2.7 is supported, whereas version 3.5 support is under development).
Python is also required for ParaViewWeb builds.

### OSMesa
Off-screen Mesa can be used as a software-renderer for running ParaView on a server without hardware OpenGL acceleration.


Retrieve the Source
-------------------
* [Install Git](git/README.md) -
  Git 1.7.2 or greater is required for development

* [Develop ParaView](git/develop.md) - Create a fork and checkout the source code.
    - A useful directory structure is ParaView/src for the git source tree, ParaView/build for the build tree, and if needed, ParaView/install for the install directory.
    - Please follow the instructions in [git/develop.md] for creating a fork and setting up the repository for contributing to ParaView.

Run CMake
---------
* `cd ParaView/build`
* `cmake-gui ../src` or `ccmake ../src`
* Generator: choose `ninja` for the ninja build system.
* There are several CMake setting you may consider changing:

| Variable | Value | Description |
| -------- | ----- | ------------|
| VTK_RENDERING_BACKEND | OpenGL2 | Use modern OpenGL |
| PARAVIEW_ENABLE_PYTHON | ON | Add python scripting support |
| BUILD_TESTING | ON/OFF | Build tests if you are contributing to ParaView |


Build
-----
* `cd ParaView/build`
* `ninja`

ParaView will be in `bin/paraview.exe`

Other Variations
----------------
[ParaViewWeb](http://kitware.github.io/paraviewweb/docs/guides/os_mesa.html) uses ParaView as a server, so doesn't require the QT gui. It configures OSMesa instead.
