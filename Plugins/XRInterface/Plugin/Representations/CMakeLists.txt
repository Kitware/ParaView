set(classes
  vtkSkyboxRepresentation
  )
set(sources)

set(xrInterfaceXMLs XRInterfaceRepresentations.xml)

if (TARGET VTK::RenderingFFMPEGOpenGL2)
  find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED QUIET COMPONENTS Multimedia)
  set(CMAKE_AUTOMOC 1)
  set(CMAKE_AUTOUIC 1)

  list(APPEND classes vtkSkyboxMovieRepresentation)
  list(APPEND xrInterfaceXMLs XRInterfaceSkyboxMovieRepresentation.xml)
endif()

if (PARAVIEW_USE_QTWEBENGINE)
  find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED QUIET COMPONENTS WebEngineWidgets Multimedia)
  set(CMAKE_AUTOMOC 1)
  set(CMAKE_AUTOUIC 1)

  list(APPEND sources vtkXRInterfaceWebView.ui)
  list(APPEND classes
    vtkXRInterfaceWebView
    vtkWebPageRepresentation
  )
  list(APPEND xrInterfaceXMLs XRInterfaceWebPageRepresentation.xml)
endif ()

vtk_module_add_module(XRInterface::vtkXRInterfaceRepresentations
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes}
  SOURCES ${sources}
  )
paraview_add_server_manager_xmls(XMLS  ${xrInterfaceXMLs})
