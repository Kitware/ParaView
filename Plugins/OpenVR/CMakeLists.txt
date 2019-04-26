find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(interfaces)
set(sources
  pvOpenVRDockPanel.cxx
  pvOpenVRDockPanel.h
  vtkPVOpenVRCollaborationClient.cxx
  vtkPVOpenVRCollaborationClient.h
  vtkPVOpenVRHelper.cxx
  vtkPVOpenVRHelper.h)


# if we build the openvr plugin then look to add collaboration
# for collaboration we leverage mineview.
#-----------------------------------------------------------------------------
if (WIN32 AND PARAVIEW_PLUGIN_ENABLE_OpenVR AND PARAVIEW_MINEVIEW_SOURCE_DIR)
  list(APPEND sources "${PARAVIEW_MINEVIEW_SOURCE_DIR}/mvCollaborationClient.cxx")
endif()

paraview_plugin_add_dock_window(
  CLASS_NAME pvOpenVRDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)
list(APPEND interfaces
  ${dock_interfaces})
list(APPEND sources
  ${dock_sources})

paraview_add_plugin(OpenVR
  VERSION "1.2"
  UI_FILES pvOpenVRDockPanel.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources})

#-----------------------------------------------------------------------------
if (WIN32 AND PARAVIEW_PLUGIN_ENABLE_OpenVR)
  option(PARAVIEW_OpenVR_COLLABORATION 
    "Turn on collaboration support for the openVR plugin, requires mineview source tree" OFF)
  mark_as_advanced(PARAVIEW_OpenVR_COLLABORATION)
endif()

if (PARAVIEW_OpenVR_COLLABORATION)
  find_path(PARAVIEW_MINEVIEW_SOURCE_DIR mvLobby.h 
    DOC "Location of your mineview source tree. See https://gitlab.kitware.com/ken-martin/mineview")
endif()

if (PARAVIEW_OpenVR_COLLABORATION AND PARAVIEW_MINEVIEW_SOURCE_DIR)
  set_property(TARGET OpenVR APPEND PROPERTY INCLUDE_DIRECTORIES
    "${PARAVIEW_MINEVIEW_SOURCE_DIR}/zeromq/release"
    "${PARAVIEW_MINEVIEW_SOURCE_DIR}"
    )
  set_property(TARGET OpenVR APPEND PROPERTY LINK_LIBRARIES
    "${PARAVIEW_MINEVIEW_SOURCE_DIR}/zeromq/release/libzmq-mt-4_3_1.lib"
    )
  set_property(SOURCE vtkPVOpenVRCollaborationClient.cxx APPEND PROPERTY
    COMPILE_DEFINITIONS OPENVR_HAS_COLLABORATION
    )
endif()

target_link_libraries(OpenVR
  PRIVATE
    ParaView::ClientServerCoreRendering
    ParaView::Core
    ParaView::ServerManagerCore
    ParaView::ServerManagerRendering
    ParaView::VTKExtensionsRendering
    ParaView::pqApplicationComponents
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::IOCore
    VTK::IOImage
    VTK::IOXML
    VTK::IOXMLParser
    VTK::InteractionWidgets
    VTK::RenderingCore
    VTK::RenderingOpenGL2
    VTK::RenderingOpenVR
    VTK::ViewsCore)
