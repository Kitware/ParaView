cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

include_directories(
  ${PARAVIEW_INCLUDE_DIRS}
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${PARAVIEW_KWSYS_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIRS}
)

# Please leave these definitions in here and do not change them.
# We use the cdi library to read both, netCDF2 and netCDF4 data.
add_definitions(-DHAVE_LIBNETCDF -DHAVE_NETCDF2 -DHAVE_NETCDF4)
set_source_files_properties(cdilib.c PROPERTY C_STANDARD 99)

vtk_module_warnings_disable(C)
vtk_module_load(vtknetcdf)

add_paraview_plugin(CDIReader "1.3"
  SERVER_MANAGER_XML
    CDIReader.xml
  SERVER_MANAGER_SOURCES
    vtkCDIReader.cxx
  SOURCES
    cdilib.c
  REQUIRED_ON_SERVER
)

target_link_libraries(CDIReader LINK_PRIVATE ${vtknetcdf_LIBRARIES})
