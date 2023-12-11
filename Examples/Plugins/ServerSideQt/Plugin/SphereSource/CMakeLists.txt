set(classes
  vtkMySphereSource)

set(sources
  SphereResources.qrc)

vtk_module_find_package(
  PACKAGE Qt5
  COMPONENTS Core)

set(CMAKE_AUTORCC ON)

vtk_module_add_module(SphereSource
  FORCE_STATIC # Using FORCE_STATIC build the vtk module statically into the plugin library, to avoid confusion when loading
  CLASSES ${classes}
  SOURCES ${sources})

vtk_module_link(SphereSource PRIVATE Qt5::Core)

paraview_add_server_manager_xmls(
  XMLS  MySphereSource.xml)
