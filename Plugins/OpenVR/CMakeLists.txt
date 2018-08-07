
# This plugin adds OpenVR support to paraview
# it requires that your VTK be built with the
# openVR module

cmake_minimum_required(VERSION 3.3)

# Find and Include ParaView
if(NOT ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif()

# Set a consistent MACOSX_RPATH default across all CMake versions.
# When CMake 2.8.12 is required, change this default to 1.
# When CMake 3.0.0 is required, remove this block (see CMP0042).
if(NOT DEFINED CMAKE_MACOSX_RPATH)
  set(CMAKE_MACOSX_RPATH 0)
endif()

IF(PARAVIEW_BUILD_QT_GUI)

  include(ParaViewQt)
  pv_find_package_qt(qt_targets REQUIRED
                QT5_COMPONENTS Widgets Network)

  pv_qt_wrap_cpp(MOC_SRCS
    pvOpenVRDockPanel.h
    )

  pv_qt_wrap_ui(UI_SRCS
    pvOpenVRDockPanel.ui
    )

  ADD_PARAVIEW_DOCK_WINDOW(
    OUTIFACES
    OUTSRCS
    CLASS_NAME pvOpenVRDockPanel
    DOCK_AREA Left
    )

  ADD_PARAVIEW_PLUGIN(OpenVR "1.1"
    GUI_INTERFACES ${OUTIFACES}
    GUI_SOURCES ${OUTSRCS} ${MOC_SRCS} ${UI_SRCS} 
    pvOpenVRDockPanel.cxx 
    vtkPVOpenVRHelper.cxx
    )

  target_link_libraries (OpenVR
    LINK_PRIVATE
      pqApplicationComponents
      vtkPVServerManagerRendering
      vtkRenderingOpenVR
      ${qt_targets}
      ${OPENVR_LIBRARY}
    )

ENDIF()
