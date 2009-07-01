cmake_minimum_required(VERSION 2.4)
cmake_policy(SET CMP0003 NEW)
# +---------------------------------------------------------------------------+
# |                                                                           |
# |                            vtkVisItDatabaseBridge                         |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
# Building the database plugin for ParaView is a two step process, namely:
#
# cd bin;
# ccmake ../ ===> Set BOOTSTRAP to ON
# make
# ccmake ../ ===> Set BOOSTRAP to OFF
# make
#
# Step 1:
#   BootstrapConfigure.
#      During this pass a local copy of the VisIt libraries and database
#      plugins are made. On Windows a copy of the "third party dependencies"
#      is also made. This is done to facilitate the crossplatform build
#      and configuration which is necessary due the large differences between
#      the VisIt build system accross platforms, and to mitigate differneces
#      in how various platforms handle shared libraries at run time. The final
#      step of this pass is to generate the CMake files, ServerManager and 
#      Pq xml for the vtkVisItDatabaseBridge plugin upon the VisIt configuration
#      found.
#
# Step 2:
#   vtkVisItDatabaseBridge ParaView Plugin.
#     During this pass the paraview plugin is built from the configuration
#     generated during the first pass.
#
Project(vtkVisItDatabaseBridge)

if (WIN32)
  set(PLUGIN_BUILD_TYPE "Release" CACHE STRING "Build type for windows builds (Release or Debug).")
endif (WIN32)

file(MAKE_DIRECTORY "${ParaView_BINARY_DIR}/bin/${PLUGIN_BUILD_TYPE}")

include("CMake/ParaViewConfig.cmake")             # Locate ParaView.
include("CMake/VisItConfig.cmake")                # Locate VisIt.
include("CMake/VisItRelocate.cmake")              # Construct a local copy of VisIt.
include("CMake/BootstrapConfig.cmake")            # Build configuration tools.

include_directories(${PARAVIEW_INCLUDE_DIRS};${PARAVIEW_GUI_INCLUDE_DIRS};${PARAVIEW_KWSYS_INCLUDE_DIRS};)
include("CMake/vtkVisItDatabase.cmake")           # Build the avt interface.
include("CMake/vtkVisItDatabaseBridge.cmake")     # Build the VTK source.
include("CMake/pqVisItDatabaseBridgePanel.cmake") # Build the custom qt panel.
include("CMake/install.cmake")                    # Install plugins and VisIt databases.
