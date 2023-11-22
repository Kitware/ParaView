find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED COMPONENTS Widgets Network)

set(required_modules "")
set(openxr_support FALSE)
set(openvr_support FALSE)

option(PARAVIEW_XRInterface_OpenXR_Support "Turn on support for OpenXR in XRInterface plugin" OFF)
if (PARAVIEW_XRInterface_OpenXR_Support)
  if (TARGET VTK::RenderingOpenXR)
    list(APPEND required_modules
      VTK::RenderingOpenXR
    )
    set(openxr_support TRUE)
  else ()
    message(FATAL_ERROR "PARAVIEW_XRInterface_OpenXR_Support is ON but VTK::RenderingOpenXR is not, please enable VTK::RenderingOpenXR for OpenXR support")
  endif ()
endif ()

option(PARAVIEW_XRInterface_OpenVR_Support "Turn on support for OpenVR in XRInterface plugin" ON)
if (PARAVIEW_XRInterface_OpenVR_Support)
  if (TARGET VTK::RenderingOpenVR)
    list(APPEND required_modules
      VTK::RenderingOpenVR
    )
    set(openvr_support TRUE)
  else ()
    message(FATAL_ERROR "PARAVIEW_XRInterface_OpenVR_Support is ON but VTK::RenderingOpenVR is not, please enable VTK::RenderingOpenVR for OpenVR support")
  endif ()
endif ()

if (NOT (PARAVIEW_XRInterface_OpenXR_Support OR PARAVIEW_XRInterface_OpenVR_Support))
  message(STATUS "XRInterface plugin is enabled but neither OpenVR or OpenXR support are enabled, the plugin may not be usable")
endif ()

option(PARAVIEW_XRInterface_OpenXRRemoting_Support  "Turn on support for OpenXR in XRInterface plugin for the Hololens2" OFF)
if (PARAVIEW_XRInterface_OpenXRRemoting_Support)
  if (TARGET VTK::RenderingOpenXRRemoting)
    list(APPEND required_modules
      VTK::RenderingOpenXRRemoting
    )
    set(openxrremoting_support TRUE)
  else ()
    message(FATAL_ERROR
      "PARAVIEW_XRInterface_OpenXRRemoting_Support is ON but VTK::RenderingOpenXRRemoting is not,
      please enable VTK::RenderingOpenXRRemoting for Hololens2 support")
  endif ()
endif ()

set(interfaces)
set(sources
  pqXRInterfaceControls.cxx
  pqXRInterfaceControls.h
  pqXRInterfaceDockPanel.cxx
  pqXRInterfaceDockPanel.h
  pqXRCustomViewpointsController.h
  pqXRCustomViewpointsController.cxx
  vtkXRInterfacePolyfill.cxx
  vtkXRInterfacePolyfill.h
  vtkPVXRInterfaceCollaborationClient.cxx
  vtkPVXRInterfaceCollaborationClient.h
  vtkPVXRInterfaceExporter.cxx
  vtkPVXRInterfaceExporter.h
  vtkPVXRInterfaceHelper.cxx
  vtkPVXRInterfaceHelper.h
  vtkPVXRInterfacePluginLocation.cxx
  vtkPVXRInterfacePluginLocation.h
  vtkPVXRInterfaceWidgets.cxx
  vtkPVXRInterfaceWidgets.h
  )

paraview_plugin_add_dock_window(
  CLASS_NAME pqXRInterfaceDockPanel
  DOCK_AREA Right
  INTERFACES dock_interfaces
  SOURCES dock_sources)

paraview_plugin_add_location(
  CLASS_NAME vtkPVXRInterfacePluginLocation
  INTERFACES location_interfaces
  SOURCES location_sources
  )

list(APPEND interfaces
  ${dock_interfaces} ${location_interfaces})
list(APPEND sources
  ${dock_sources} ${location_sources})

set(openxrremoting_eula_file "")
if (openxrremoting_support)
  set (openxrremoting_eula_file "OpenXRRemoting/EULA.txt")
endif()

paraview_add_plugin(XRInterface
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.2"
  EULA ${openxrremoting_eula_file}
  UI_FILES pqXRInterfaceDockPanel.ui pqXRInterfaceControls.ui
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  MODULES XRInterface::vtkXRInterfaceRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF
  )

#-----------------------------------------------------------------------------
cmake_dependent_option(PARAVIEW_XRInterface_Imago_Support
    "Turn on support for the Imago Image Loading" OFF "PARAVIEW_PLUGIN_ENABLE_XRInterface;WIN32" OFF)
mark_as_advanced(PARAVIEW_XRInterface_Imago_Support)

set_property(
  SOURCE
  pqXRInterfaceDockPanel.cxx
  vtkPVXRInterfaceWidgets.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "XRINTERFACE_HAS_IMAGO_SUPPORT=$<BOOL:${PARAVIEW_XRInterface_Imago_Support}>")

set(xr_input_files
  pv_openvr_actions.json
  pv_openvr_binding_hpmotioncontroller.json
  pv_openvr_binding_knuckles.json
  pv_openvr_binding_vive_controller.json
  pv_openvr_binding_oculus_touch.json
)

set_property(
  SOURCE
  pqXRInterfaceDockPanel.cxx
  vtkPVXRInterfaceHelper.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "XRINTERFACE_HAS_OPENXR_SUPPORT=$<BOOL:${openxr_support}>"
    COMPILE_DEFINITIONS "XRINTERFACE_HAS_OPENXRREMOTING_SUPPORT=$<BOOL:${openxrremoting_support}>")

set_property(
  SOURCE
  pqXRInterfaceDockPanel.cxx
  vtkPVXRInterfaceHelper.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "XRINTERFACE_HAS_OPENVR_SUPPORT=$<BOOL:${openvr_support}>")

if (openxr_support)
  list(APPEND xr_input_files
    pv_openxr_actions.json
    pv_openxr_binding_khr_simple_controller.json
    pv_openxr_binding_knuckles.json
    pv_openxr_binding_vive_controller.json
    pv_openxr_binding_oculus_touch_controller.json
    pv_openxr_binding_microsoft_hand_interaction.json
  )
endif()

set_property(
  SOURCE
  vtkPVXRInterfaceCollaborationClient.cxx
  pqXRInterfaceControls.cxx
  APPEND PROPERTY
    COMPILE_DEFINITIONS "XRINTERFACE_HAS_COLLABORATION=$<BOOL:${VTK_ENABLE_VR_COLLABORATION}>")

foreach(inputfile IN LISTS xr_input_files)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/${inputfile}
    "${CMAKE_BINARY_DIR}/${_paraview_build_plugin_directory}/${inputfile}"
    COPYONLY)
endforeach()

install(
  FILES ${xr_input_files}
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

target_link_libraries(XRInterface
  PRIVATE
  ${required_modules}
)
target_compile_definitions(XRInterface PRIVATE QT_NO_KEYWORDS)

# if we have ffmpeg enabled then add in the dependency
# and set a define for the exporter to handle exporting movies
set(have_ffmpeg 0)
if (TARGET VTK::RenderingFFMPEGOpenGL2)
  set(have_ffmpeg 1)
  target_link_libraries(XRInterface
    PRIVATE
      VTK::RenderingFFMPEGOpenGL2
    )
endif()
set_property(SOURCE vtkPVXRInterfaceExporter.cxx APPEND PROPERTY
  COMPILE_DEFINITIONS "PARAVIEW_ENABLE_FFMPEG=$<BOOL:${have_ffmpeg}>")

set_property(SOURCE vtkPVXRInterfaceWidgets.cxx APPEND PROPERTY
  COMPILE_DEFINITIONS "PARAVIEW_USE_QTWEBENGINE=$<BOOL:${PARAVIEW_USE_QTWEBENGINE}>")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
