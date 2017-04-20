## Thanks to John Biddiscombe of
## CSCS - Swiss National Supercomputing Centre for creating and contributing
## this plugin.

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

if(PARAVIEW_USE_MPI)
  include(vtkMPI)
endif()

set(Module_SRCS
    vtkH5PartReader.cxx
    H5Part/src/H5Part.c
    H5Part/src/H5Block.c
    )
vtk_module_library(vtkPVVTKExtensionsH5PartReader ${Module_SRCS})

if (PARAVIEW_USE_MPI)
  vtk_mpi_link(vtkPVVTKExtensionsH5PartReader)
endif()

set_source_files_properties(
  ${Module_SRCS}
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
    SKIP_HEADER_INSTALL 1)

SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src/H5Part.c
  ${CMAKE_CURRENT_SOURCE_DIR}/H5Part/src/H5Block.c
  PROPERTIES LANGUAGE CXX
)
