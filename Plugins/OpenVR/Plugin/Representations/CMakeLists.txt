set(classes
  vtkSkyboxRepresentation
  )
set(sources)

set(openvrXMLs OpenVRRepresentations.xml)

if (PARAVIEW_ENABLE_FFMPEG AND TARGET VTK::RenderingFFMPEGOpenGL2)
  find_package(Qt5 REQUIRED QUIET COMPONENTS Multimedia)
  list(APPEND classes vtkSkyboxMovieRepresentation)
  list(APPEND openvrXMLs  OpenVRSkyboxMovieRepresentation.xml)
endif()

if (PARAVIEW_USE_QTWEBENGINE)
  find_package(Qt5 REQUIRED QUIET COMPONENTS WebEngineWidgets Multimedia)
  list(APPEND sources vtkOpenVRWebView.ui)
  list(APPEND classes
    vtkOpenVRWebView
    vtkWebPageRepresentation
  )
  list(APPEND openvrXMLs OpenVRWebPageRepresentation.xml)
endif ()

set(CMAKE_AUTOMOC 1)
set(CMAKE_AUTOUIC 1)

vtk_module_add_module(OpenVR::vtkOpenVRRepresentations
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes}
  SOURCES ${sources}
  )
paraview_add_server_manager_xmls(XMLS  ${openvrXMLs})
