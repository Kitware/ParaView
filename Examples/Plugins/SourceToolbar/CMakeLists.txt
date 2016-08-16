

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
  # We need to wrap for Qt stuff such as signals/slots etc. to work correctly.
  IF (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    QT5_WRAP_CPP(MOC_SRCS SourceToolbarActions.h)
  ELSE ()
    QT4_WRAP_CPP(MOC_SRCS SourceToolbarActions.h)
  ENDIF ()

  # This is a macro for adding QActionGroup subclasses automatically as toolbars.
  ADD_PARAVIEW_ACTION_GROUP(IFACES IFACE_SRCS
                            CLASS_NAME SourceToolbarActions
                            GROUP_NAME "ToolBar/SourceToolbar")

  # Now crete a plugin for the toolbar. Here we pass IFACES and IFACE_SRCS
  # which are filled up by the above macro with relevant entries
  ADD_PARAVIEW_PLUGIN(SourceToolbar "1.0"
                      GUI_INTERFACES ${IFACES}
                      SOURCES ${MOC_SRCS} ${IFACE_SRCS}
                      SourceToolbarActions.cxx)
ENDIF()
