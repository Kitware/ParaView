# Bring in VTK's MPI macros for MPI dependency
include(vtkMPI)

# Resolve GenericIO and CosmoTools external dependencies
find_package(GenericIO REQUIRED)
find_package(CosmoTools REQUIRED)

# Add external include directories
include_directories(
    ${COSMOTOOLS_INCLUDE_DIR}
    ${GENERIC_IO_INCLUDE_DIR}
    )

# Define external libraries used by this module
set(CosmoToolsExternalLibs
    ${COSMOTOOLS_LIBRARIES}
    ${GENERIC_IO_LIBRARIES}
    )

# Set external includes for the CosmoTools module
set(vtkPVVTKExtensionsCosmoTools_INCLUDE_DIRS
    ${COSMOTOOLS_INCLUDE_DIR}
    ${GENERIC_IO_INCLUDE_DIR}
    )

# advanced option for whether to use voronoi.h from cosmotools
option(COSMOTOOLS_HAS_VORONOI OFF)
mark_as_advanced(COSMOTOOLS_HAS_VORONOI)

# List of module sources
set(Module_SRCS

  ## Readers
  vtkACosmoReader.cxx
  vtkPCosmoReader.cxx
  vtkPGenericIOReader.cxx
  vtkPGenericIOMultiBlockReader.cxx
  vtkPMultiResolutionGenericIOReader.cxx

  ## Filters
  vtkMinkowskiFilter.cxx
  vtkPLANLHaloFinder.cxx
  vtkPMergeConnected.cxx
  )

if (${COSMOTOOLS_HAS_VORONOI})
  list(APPEND Module_SRCS vtkPVoronoiReader.cxx)
endif()

vtk_module_library(vtkPVVTKExtensionsCosmoTools ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsCosmoTools
                      LINK_PRIVATE
                        ${CosmoToolsExternalLibs}
                        pthread
                      )
vtk_mpi_link(vtkPVVTKExtensionsCosmoTools)
