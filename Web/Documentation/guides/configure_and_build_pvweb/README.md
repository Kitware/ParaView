# ParaViewWeb Source Configure and Build


## Introduction

This document describes the process of configuring and building ParaView from source.  
Useful information can always be found at the <a href="http://www.paraview.org/" target="_blank">ParaView homepage.</a>


##Required Tools
### The CMake cross-platform, open-source build system.

CMake binaries and source code are available at <a href="http://www.cmake.org/cmake/resources/software.html" target="_blank">CMake Releases</a>.  
Many linux style distributions include CMake in their package management systems.


### Git

The ParaView source code is hosted in a git repository or can be obtained in archived format. If you intend to edit the source code and submit patches/features, you will need git.

####Linux
Git is usually available through the package management systems or you can build from source code <a href="http://git-scm.com/downloads" target="_blank">Here</a>.

####Windows
Install <a href="http://msysgit.github.io/" target="_blank">msysgit</a>

##Obtaining source code
###Zip/Tar file

<a href="http://www.paraview.org/paraview/resources/software.php" target="_blank">ParaView Source Code</a>

###With Git

Further information is provided at the <a href="http://www.paraview.org/Wiki/ParaView/Git" target="_blank">ParaView/Git Wiki</a>


##Configure with CMake

Create a build directory - The preferred structure when building ParaView is to separate your source code from your build directory.

####Linux users:

1. In a terminal cd to your build_dir.
2. Run `ccmake path/to/source-code` which opens a curses dialog
3. Press 'c' to configure. Configuration needs to occur until issues are resolved and cmake presents the option to generate 'g'.

####Windows users:
1. Open cmake-gui.
2. Select your compiler.
3. Fill in the path to your source code and build directories in the appropriate boxes.
4. Click the configure button. Configuration needs to occur until issues are resolved and cmake presents the generate button.

From this point, the process between Linux and Windows is basically the same. The following options provides the minor deviations from the default configuration and are sufficient to build ParaView for the web.  

BUILD_TESTING - While the testing code is very useful, it can take a long time to compile. Users looking for a shorter build time can disable testing code by setting this option to OFF.  
CMAKE_BUILD_TYPE - Options include Debug, Release and RelWithDebInfo. Set appropriately  
PARAVIEW_BUILD_QT_GUI - Used to build the Qt user interface. Web developers can safely set this option OFF. Setting to ON will require a build of Qt 4.8 which is out of the scope of this document.  
PARAVIEW_ENABLE_CATALYST - OFF  
PARAVIEW_ENABLE_PYTHON - Builds ParaView's python bindings. You will need python 2.7 binaries and header files (devel packages )installed for this option.  
VTK_Group_ParaViewCore - ON  
VTK_Group_ParaViewPython - ON  

CMake Configure  

Under advanced options:  
PARAVIEW_BUILD_WEB_DOCUMENTATION - ON will build ParaView's Web Documentation targets. This option requires <a href="https://github.com/senchalabs/jsduck" traget="_blank">jsduck</a>  
Turn off plugins - If not required, it can be helpful to turn off plugins you don't need.  
CMake Configure  

Build using your compiler.

##Usage


##Troubleshooting
Coming Soon
