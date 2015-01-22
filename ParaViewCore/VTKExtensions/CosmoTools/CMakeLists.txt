# Bring in VTK's MPI macros for MPI dependency
include(vtkMPI)

# Resolve GenericIO and CosmoTools external dependencies
find_package(GenericIO REQUIRED)

# Add external include directories
include_directories(
#    ${COSMOTOOLS_INCLUDE_DIR}
    ${GENERIC_IO_INCLUDE_DIR}
    )

# Define external libraries used by this module
set(CosmoToolsExternalLibs
#    ${COSMOTOOLS_LIBRARIES}
    ${GENERIC_IO_LIBRARIES}
    )

# Set external includes for the CosmoTools module
set(vtkPVVTKExtensionsCosmoTools_INCLUDE_DIRS
#    ${COSMOTOOLS_INCLUDE_DIR}
    ${GENERIC_IO_INCLUDE_DIR}
    )

# List of module sources
set(Module_SRCS

  ## Readers
  vtkACosmoReader.cxx
  vtkPCosmoReader.cxx
  vtkPGenericIOReader.cxx
  vtkGenericIOUtilities.cxx
  vtkPGenericIOMultiBlockReader.cxx
  vtkPGenericIOMultiBlockWriter.cxx
  vtkPMultiResolutionGenericIOReader.cxx

  ## Filters
  vtkMinkowskiFilter.cxx
  vtkPLANLHaloFinder.cxx
  vtkPMergeConnected.cxx
  )

set_source_files_properties(
  vtkGenericIOUtilities.cxx
  WRAP_EXCLUDE
  )

vtk_module_library(vtkPVVTKExtensionsCosmoTools ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsCosmoTools
                      LINK_PRIVATE
                        ${CosmoToolsExternalLibs}
                        pthread
                      )
vtk_mpi_link(vtkPVVTKExtensionsCosmoTools)
