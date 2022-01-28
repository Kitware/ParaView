find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(required_modules "")
set(openxr_support FALSE)

if (TARGET VTK::RenderingOpenXR)
  list(APPEND required_modules 
    VTK::RenderingOpenXR
  )
  find_package(OpenXR REQUIRED)
  set(openxr_support TRUE)
endif()

if (TARGET VTK::RenderingOpenVR)
  list(APPEND required_modules 
    VTK::RenderingOpenVR
  )
endif()

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

set_property(
  SOURCE
    pqOpenVRDockPanel.cxx
    vtkPVOpenVRWidgets.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "OPENVR_HAS_IMAGO_SUPPORT=$<BOOL:${PARAVIEW_OpenVR_Imago_Support}>")

set(vr_input_files
  pv_openvr_actions.json
  pv_openvr_binding_hpmotioncontroller.json
  pv_openvr_binding_vive_controller.json
  pv_openvr_binding_oculus_touch.json
)

set_property(
  SOURCE
    pqOpenVRDockPanel.cxx
    vtkPVOpenVRHelper.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "PARAVIEW_HAS_OPENXR_SUPPORT=$<BOOL:${openxr_support}>")

if (openxr_support)
  list(APPEND vr_input_files 
    pv_openxr_actions.json
    pv_openxr_binding_khr_simple_controller.json
    pv_openxr_binding_valve_index_controller.json
    pv_openxr_binding_vive_controller.json
  )
endif()

set_property(
  SOURCE 
    vtkPVOpenVRCollaborationClient.cxx 
  APPEND PROPERTY
    COMPILE_DEFINITIONS "OPENVR_HAS_COLLABORATION=$<BOOL:${VTK_ENABLE_VR_COLLABORATION}>")

foreach(inputfile IN LISTS vr_input_files)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}
    "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_directory}/${inputfile}"
    COPYONLY)
endforeach()

install(
  FILES ${vr_input_files}
  DESTINATION "${_paraview_build_plugin_directory}"
  COMPONENT   "${_paraview_build_PLUGINS_COMPONENT}"
)

list(APPEND required_modules
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
  VTK::RenderingVR
  VTK::ViewsCore
)

target_link_libraries(OpenVR
  PRIVATE
  ${required_modules}
)
target_compile_definitions(OpenVR PRIVATE QT_NO_KEYWORDS)

# if we have ffmpeg enabled then add in the dependency
# and set a define for the exporter to handle exporting movies
set(have_ffmpeg 0)
if (TARGET VTK::RenderingFFMPEGOpenGL2)
  set(have_ffmpeg 1)
  target_link_libraries(OpenVR
    PRIVATE
      VTK::RenderingFFMPEGOpenGL2
    )
endif()
set_property(SOURCE vtkPVOpenVRExporter.cxx APPEND PROPERTY
  COMPILE_DEFINITIONS "PARAVIEW_ENABLE_FFMPEG=$<BOOL:${have_ffmpeg}>")

set_property(SOURCE vtkPVOpenVRWidgets.cxx APPEND PROPERTY
  COMPILE_DEFINITIONS "PARAVIEW_USE_QTWEBENGINE=$<BOOL:${PARAVIEW_USE_QTWEBENGINE}>")
