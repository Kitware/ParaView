
# create a plugin that implements an object panel for a source proxy

cmake_minimum_required(VERSION 3.3)

IF(NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

IF(PARAVIEW_BUILD_QT_GUI)
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(MOC_SRCS MyToolBarActions.h)
  ELSE ()
    QT4_WRAP_CPP(MOC_SRCS MyToolBarActions.h)
  ENDIF ()

  # we implement a pqConePanel.h for the ConeSource
  ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS CLASS_NAME MyToolBarActions
                            GROUP_NAME "ToolBar/MyActions")

  # create a plugin for this panel
  ADD_PARAVIEW_PLUGIN(GUIMyToolBar "1.0"
                      GUI_INTERFACES ${IFACES}
                      SOURCES MyToolBarActions.cxx ${MOC_SRCS} ${IFACE_SRCS})
ENDIF()
