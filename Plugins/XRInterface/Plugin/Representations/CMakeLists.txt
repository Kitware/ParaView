set(classes
  vtkSkyboxRepresentation
  )
set(sources)

set(xrInterfaceXMLs XRInterfaceRepresentations.xml)

set(qt_required_components)

_vtk_module_optional_dependency_exists(VTK::RenderingFFMPEGOpenGL2
  SATISFIED_VAR _have_vtk_renderingffmpegopengl2)

if (_have_vtk_renderingffmpegopengl2)
  list(APPEND qt_required_components
    Multimedia)

  list(APPEND classes vtkSkyboxMovieRepresentation)
  list(APPEND xrInterfaceXMLs XRInterfaceSkyboxMovieRepresentation.xml)
endif()

if (PARAVIEW_USE_QTWEBENGINE)
  list(APPEND qt_required_components
    WebEngineWidgets
    Multimedia)
  set(CMAKE_AUTOUIC 1)

  list(APPEND sources vtkXRInterfaceWebView.ui)
  list(APPEND classes
    vtkXRInterfaceWebView
    vtkWebPageRepresentation
  )
  list(APPEND xrInterfaceXMLs XRInterfaceWebPageRepresentation.xml)
endif ()

set(qt_targets)

if (qt_required_components)
  list(REMOVE_DUPLICATES qt_required_components)
  find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED
    COMPONENTS ${qt_required_components})
  foreach (qt_required_component IN LISTS qt_required_components)
    list(APPEND qt_targets
      "Qt${PARAVIEW_QT_MAJOR_VERSION}::${qt_required_component}")
  endforeach ()
  set(CMAKE_AUTOMOC 1)
endif ()

vtk_module_add_module(XRInterface::vtkXRInterfaceRepresentations
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes}
  SOURCES ${sources}
  )
vtk_module_link(XRInterface::vtkXRInterfaceRepresentations
  PRIVATE
    ${qt_targets})
paraview_add_server_manager_xmls(XMLS  ${xrInterfaceXMLs})
