set(classes
  vtkCDIReader)

set (sources
    cdi_tools.cxx
    projections.cxx
    )
set (private_headers
    cdi_tools.h
    projections.h)

vtk_module_add_module(CDIReader::vtkCDIReader
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)

vtk_module_find_package(
  PACKAGE CDI
  VERSION 2.1.0
  )

vtk_module_link(CDIReader::vtkCDIReader
  PRIVATE
  CDI::CDI)

if (CDI_VERSION VERSION_GREATER_EQUAL 2.4.1)
  vtk_module_definitions(CDIReader::vtkCDIReader PRIVATE CDI_241_VLIST_API)
endif ()  

paraview_add_server_manager_xmls(
  XMLS  CDIReader.xml)
