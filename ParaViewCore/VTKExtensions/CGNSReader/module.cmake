set(cgns_private_depends)
list(APPEND cgns_private_depends vtksys vtkParallelMPI)

cmake_dependent_option(CGNS_LINK_TO_HDF5
  "use vtkhdf5_LIBRARIES for linking cgns to hdf5" ON
  "PARAVIEW_ENABLE_CGNS" ON)
mark_as_advanced(CGNS_LINK_TO_HDF5)

if (CGNS_LINK_TO_HDF5)
  list(APPEND cgns_private_depends vtkhdf5)
endif()

vtk_module(vtkPVVTKExtensionsCGNSReader
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
      vtkPVVTKExtensionsCore
    PRIVATE_DEPENDS
      ${cgns_private_depends}
    KIT
      vtkPVExtensions
)

set_property(GLOBAL PROPERTY
    vtkPVVTKExtensionsCGNSReader_SERVERMANAGER_XMLS
    
    ${CMAKE_CURRENT_LIST_DIR}/resources/CGNSReader.xml
)
