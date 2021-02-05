set(classes
  vtkCDIReader)
if (PARAVIEW_USE_QT)
   find_package(Qt5 COMPONENTS Widgets)  
endif()




set (sources
    vtkCDIReader.cxx
    vtkCDIReader.h
    cdi_tools.cxx
    cdi_tools.h
    helpers.cxx
    helpers.h
    projections.cxx
    projections.h
    )

vtk_module_add_module(CDIReader::vtkCDIReader
  CLASSES ${classes}
  SOURCES ${sources}
)

if (TARGET VTK::ParallelMPI)
  # FIXME: The API should not depend on this.
  vtk_module_definitions(CDIReader::vtkCDIReader
    PUBLIC
      PARAVIEW_USE_MPI)
endif ()


find_package(CDI)
message(STATUS "Use CDI version ${CDI_VERSION}")

vtk_module_include(CDIReader::vtkCDIReader
    PRIVATE $<BUILD_INTERFACE:${CDI_INCLUDE_DIRS}>)
vtk_module_link(CDIReader::vtkCDIReader
    PRIVATE ${CDI_LIBRARIES})


if (PARAVIEW_BUILD_CDI_WITH_GRIB)  
  vtk_module_include(CDIReader::vtkCDIReader
    PRIVATE $<BUILD_INTERFACE:${ECCODES_INCLUDE_DIRS}>)
  vtk_module_link(CDIReader::vtkCDIReader
    PRIVATE ${ECCODES_LIBRARIES})
endif ()

paraview_add_server_manager_xmls(
  XMLS  CDIReader.xml)
