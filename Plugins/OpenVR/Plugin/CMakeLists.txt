find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(interfaces)
set(sources
  pqOpenVRControls.cxx
  pqOpenVRControls.h
  pqOpenVRDockPanel.cxx
  pqOpenVRDockPanel.h
  vtkOpenVRPolyfill.cxx
  vtkOpenVRPolyfill.h
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
  MODULES OpenVR::vtkOpenVRRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF
  )

#-----------------------------------------------------------------------------
cmake_dependent_option(PARAVIEW_OpenVR_Imago_Support
    "Turn on support for the Imago Image Loading" OFF "PARAVIEW_PLUGIN_ENABLE_OpenVR;WIN32" OFF)
mark_as_advanced(PARAVIEW_OpenVR_Imago_Support)

if (PARAVIEW_OpenVR_Imago_Support)
  set_property(
    SOURCE
      pqOpenVRDockPanel.cxx
      vtkPVOpenVRWidgets.cxx
    APPEND PROPERTY
      COMPILE_DEFINITIONS OPENVR_HAS_IMAGO_SUPPORT)
endif()

if (VTK_ENABLE_VR_COLLABORATION)
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
  DESTINATION "${_paraview_build_plugin_directory}"
  COMPONENT   "${_paraview_build_PLUGINS_COMPONENT}"
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
