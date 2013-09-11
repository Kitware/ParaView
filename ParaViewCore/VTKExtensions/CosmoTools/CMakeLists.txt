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
    ${GENERIC_IO_LIBRARIES}
    ${COSMOTOOLS_LIBRARIES}
    )

# Set external includes for the CosmoTools module
set(vtkPVVTKExtensionsCosmoTools_INCLUDE_DIRS
    ${COSMOTOOLS_INCLUDE_DIR}
    ${GENERIC_IO_INCLUDE_DIR}
    )

# List of module sources
set(Module_SRCS
  vtkACosmoReader.cxx
  vtkPCosmoReader.cxx
  vtkPGenericIOReader.cxx
  vtkPLANLHaloFinder.cxx
  )

vtk_module_library(vtkPVVTKExtensionsCosmoTools ${Module_SRCS})
target_link_libraries(vtkPVVTKExtensionsCosmoTools
                      LINK_PRIVATE ${CosmoToolsExternalLibs}
                      )
vtk_mpi_link(vtkPVVTKExtensionsCosmoTools)