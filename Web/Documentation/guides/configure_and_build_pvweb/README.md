# ParaViewWeb Source Configure and Build


## Introduction

This document describes the process of configuring and building ParaView from source.
This document describes the process of configuring and building ParaView from source. Useful information can always be found at the [ParaView homepage](http://www.paraview.org/ "ParaView Homepage")


##Required Tools
### The CMake cross-platform, open-source build system.
    CMake binaries and source code are available at [cmake](http://www.cmake.org/cmake/resources/software.html "CMake"). Many linux style distrobutions include CMake in their package management systems.

### Git
The ParaView source code is hosted in a git repo or can be obtained in archived format. If you intend to edit the source code and submit patches/features, you will need git.

####Windows
        Install [msysgit](http://msysgit.github.io/ "msysgit")

####Linux
        Usually available through the package management systems or can be build git from source code [here](http://git-scm.com/downloads "git").



##Obtaining source code
###Zip/Tar file
        [ParaView Source](http://www.paraview.org/paraview/resources/software.php "ParaView Source Code")
###With Git
        Further information is provided at the [ParaView/Git Wiki](http://www.paraview.org/Wiki/ParaView/Git "ParaView/Git Wiki")




##Configure with CMake
Create a build directory - The preferred structure when building VTK is to separate your source code from your build directory.

Linux users:
      In a terminal cd to your build_dir.
      Run `ccmake path/to/source-code` which opens a curses dialog
      Press 'c' to configure. Configuration needs to occur until issues are resolved and cmake presents the option to generate 'g'.

Windows users:
      Open cmake-gui.
      Select your compiler.
      Fill in the path to your source code and build directories in the appropriate boxes.
      Click the configure button. Configuration needs to occur until issues are resolved and cmake presents the generate button.

From this point, the process between Linux and Windows is basically the same. The following options are sufficient to build VTK for the web. The following list provides the minor deviations from the default configuration.

     BUILD_TESTING - While the testing code is very useful, it can take a long time to compile. Users looking for a shorter build time can disable testing code by setting this option to OFF.
     CMAKE_BUILD_TYPE - Options include Debug, Release and RelWithDebInfo. Set appropriately
     PARAVIEW_BUILD_QT_GUI - Used to build the Qt user interface. Web developers can safely set this option OFF. Setting to ON will require a build of Qt 4.8 which is out of the scope of this document.
     PARAVIEW_ENABLE_CATALYST - OFF
     PARAVIEW_ENABLE_PYTHON - Builds ParaView's python bindings. You will need python 2.7 binaries and header files (devel packages )installed for this option.
     VTK_Group_ParaViewCore
     VTK_Group_ParaViewPython

     CMake Configure

     Under advanced options:
     PARAVIEW_BUILD_WEB_DOCUMENTATION - Turn ON to build ParaView's Web Documentation targets. This option requires [jsduck](https://github.com/senchalabs/jsduck)
     Turn off plugins - If not required, it can be helpful to turn plugins you don't need.
     CMake Configure

     Build using your compiler.


##Usage


##Troubleshooting
Coming Soon
