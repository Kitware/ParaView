find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(interfaces)
set(sources
  pqOpenVRControls.cxx
  pqOpenVRControls.h
  pqOpenVRDockPanel.cxx
  pqOpenVRDockPanel.h
  vtkOpenVRPolyfill.h
  vtkOpenVRPolyfill.cxx
  vtkPVOpenVRCollaborationClient.cxx
  vtkPVOpenVRCollaborationClient.h
  vtkPVOpenVRExporter.cxx
  vtkPVOpenVRExporter.h
  vtkPVOpenVRHelper.cxx
  vtkPVOpenVRHelper.h
  vtkPVOpenVRPluginLocation.cxx
  vtkPVOpenVRPluginLocation.h
  vtkPVOpenVRWidgets.cxx
  vtkPVOpenVRWidgets.h
  )


# if we build the openvr plugin then look to add collaboration
# for collaboration we leverage mineview.
#-----------------------------------------------------------------------------
if (WIN32 AND
    PARAVIEW_MINEVIEW_SOURCE_DIR AND
    PARAVIEW_OpenVR_COLLABORATION)
  list(APPEND sources "${PARAVIEW_MINEVIEW_SOURCE_DIR}/mvCollaborationClient.cxx")
endif()


paraview_plugin_add_dock_window(
  CLASS_NAME pqOpenVRDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)

paraview_plugin_add_location(
  CLASS_NAME vtkPVOpenVRPluginLocation
  INTERFACES location_interfaces
  SOURCES location_sources
  )

list(APPEND interfaces
  ${dock_interfaces} ${location_interfaces})
list(APPEND sources
  ${dock_sources} ${location_sources})

paraview_add_plugin(OpenVR
  VERSION "1.2"
  UI_FILES pqOpenVRDockPanel.ui pqOpenVRControls.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  )


#-----------------------------------------------------------------------------
if (WIN32 AND PARAVIEW_PLUGIN_ENABLE_OpenVR)
  option(PARAVIEW_OpenVR_COLLABORATION
    "Turn on collaboration support for the openVR plugin, requires mineview source tree" OFF)
  mark_as_advanced(PARAVIEW_OpenVR_COLLABORATION)
  option(PARAVIEW_OpenVR_Imago_Support
    "Turn on support for the Imago Image Loading" OFF)
  mark_as_advanced(PARAVIEW_OpenVR_Imago_Support)
endif()

if (PARAVIEW_OpenVR_Imago_Support)
  set_property(
    SOURCE
      pqOpenVRDockPanel.cxx
      vtkPVOpenVRWidgets.cxx
    APPEND PROPERTY
      COMPILE_DEFINITIONS OPENVR_HAS_IMAGO_SUPPORT)
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

set(openvr_input_files
  pv_openvr_actions.json
  pv_openvr_binding_hpmotioncontroller.json
  pv_openvr_binding_vive_controller.json
  pv_openvr_binding_oculus_touch.json
)

foreach(inputfile IN LISTS openvr_input_files)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}
    "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_directory}/${inputfile}"
    COPYONLY)
endforeach()

install(
  FILES ${openvr_input_files}
  DESTINATION ${_paraview_build_plugin_directory}
  COMPONENT   ${_paraview_build_PLUGINS_COMPONENT}
)

target_link_libraries(OpenVR
  PRIVATE
    ParaView::pqApplicationComponents
    ParaView::RemotingCore
    ParaView::RemotingServerManager
    ParaView::RemotingViews
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::CommonSystem
    VTK::IOCore
    VTK::IOImage
    VTK::IOXML
    VTK::IOXMLParser
    VTK::InteractionWidgets
    VTK::RenderingCore
    VTK::RenderingOpenGL2
    VTK::RenderingOpenVR
    VTK::ViewsCore)
