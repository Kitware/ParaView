# VTKWeb Source Configure and Build


## Introduction

This document describes the process of configuring and building VTK from source. Useful information can always be found on the [VTK Homepage](http://www.vtk.org).

## Required Tools

### The CMake cross-platform, open-source build system.

CMake binaries and source code are available here: [CMake Releases](http://www.cmake.org/cmake/resources/software.html).
Many Linux style distributions include CMake in their package management systems.


### Git

The VTK source code is hosted in a git repository. It can also be obtained in archived format. If you intend to edit the source code and submit patches/features, you will need git.

#### Linux

Git is usually available through the package management systems. You can also build from the source code found [Here](http://git-scm.com/downloads).

#### Windows

Install [msysgit](http://msysgit.github.io/)

## Obtaining source code


### Zip/Tar file

[VTK Source Code](http://www.vtk.org/VTK/resources/software.html)

### With Git

Additional information is provided on the [VTK/Git Wiki](http://www.vtk.org/Wiki/VTK/Git)


## Configure with CMake

Create a build directory - The preferred structure when building VTK is to separate your source code from your build directory.

#### Linux users:

1. In a terminal cd to your build_dir.
2. Run `ccmake path/to/source-code` that opens a curses dialog
3. Press 'c' to configure. Configuration needs to occur until issues are resolved and CMake presents the option to generate 'g'.

#### Windows users:
1. Open cmake-gui.
2. Select your compiler.
3. Fill in the path to your source code and build directories in the appropriate boxes.
4. Click the configure button. Configuration needs to occur until issues are resolved and CMake presents the generate button.

From this point forward, the process between Linux and Windows is basically the same. The following options provide minor deviations from the default configuration and are sufficient to build VTK for the web.

| CMake option        | Recommended value | Comment   |
|:-------------------:|:-----------------:|:----------|
| BUILD_DOCUMENTATION | OFF     | |
| BUILD_TESTING       | OFF     | While the testing code is very useful, it can take a long time to compile. Users looking for a shorter build time can disable testing code by setting this option to OFF. |
| CMAKE_BUILD_TYPE    | Release | Options include Debug, Release, and RelWithDebInfo. Set for your needs. |
| VTK_Group_Web       | ON      | Mandatory for VTK-Web |
| VTK_WRAP_PYTHON     | ON      | Builds the python bindings. You will need python 2.7 binaries and header files (devel packages )installed for this option. |

CMake Configure.  
Build using your compiler.  

##Contributions

Details of the VTK software process can be obtained from [The VTK Software Process Guide](https://docs.google.com/a/kitware.com/document/d/1nzinw-dR5JQRNi_gb8qwLL5PnkGMK2FETlQGLr10tZw/edit)
