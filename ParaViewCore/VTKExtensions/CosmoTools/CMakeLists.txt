# Bring in VTK's MPI macros for MPI dependency
include(vtkMPI)

# Resolve the GenericIO external dependency
find_package(GenericIO REQUIRED)

# Add external include directories
include_directories(
    ${GENERIC_IO_INCLUDE_DIR}
    )

# Define external libraries used by this module
set(CosmoToolsExternalLibs
    ${GENERIC_IO_LIBRARIES}
    )

# Set external includes for the CosmoTools module
set(vtkPVVTKExtensionsCosmoTools_INCLUDE_DIRS
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
  vtkPANLHaloFinder.cxx
  vtkPANLSubhaloFinder.cxx
  vtkPMergeConnected.cxx
  )

vtk_module_export_code_find_package(GenericIO)

set_source_files_properties(
  vtkGenericIOUtilities.cxx
  WRAP_EXCLUDE
  )
set_source_files_properties(
  vtkGenericIOUtilities
  PROPERTIES WRAP_EXCLUDE_PYTHON 1)
vtk_module_library(vtkPVVTKExtensionsCosmoTools ${Module_SRCS})
vtk_module_link_libraries(vtkPVVTKExtensionsCosmoTools
                      LINK_PRIVATE
                        ${CosmoToolsExternalLibs}
                        pthread
                      )
vtk_mpi_link(vtkPVVTKExtensionsCosmoTools)
