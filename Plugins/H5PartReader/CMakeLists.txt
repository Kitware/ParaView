## Thanks to John Biddiscombe of
## CSCS - Swiss National Supercomputing Centre for creating and contributing
## this plugin.

vtk_module_load(vtkhdf5)

INCLUDE_DIRECTORIES(
  ${vtkhdf5_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src
  )

if(PARAVIEW_USE_MPI)
  ADD_DEFINITIONS(-DH5PART_HAS_MPI)
endif()

ADD_DEFINITIONS(-DH5_USE_16_API)

# needed for mpich
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

ADD_PARAVIEW_PLUGIN(H5PartReader "1.0"
  SERVER_MANAGER_SOURCES
    vtkH5PartReader.cxx

  SERVER_SOURCES
    H5Part/src/H5Part.c
    H5Part/src/H5Block.c

  SERVER_MANAGER_XML
    H5PartServerManager.xml

  REQUIRED_ON_SERVER
)

target_link_libraries(H5PartReader LINK_PRIVATE
  ${vtkhdf5_LIBRARIES}
  vtksys)

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
  vtk_mpi_link(H5PartReader)
endif()

SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src/H5Part.c
  ${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src/H5Block.c
  PROPERTIES LANGUAGE CXX
)
