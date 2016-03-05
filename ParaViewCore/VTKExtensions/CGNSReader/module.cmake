cmake_dependent_option(CGNS_LINK_TO_HDF5
  "use vtkhdf5_LIBRARIES for linking cgns to hdf5" ON
  "PARAVIEW_ENABLE_CGNS" ON)
mark_as_advanced(CGNS_LINK_TO_HDF5)

set(cgns_private_depends)
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
      vtksys
      vtkParallelCore
      ${cgns_private_depends}
    TEST_DEPENDS
      vtkInteractionStyle
      vtkTestingCore
      vtkTestingRendering
    TEST_LABELS
      PARAVIEW
    KIT
      vtkPVExtensions
)

set_property(GLOBAL PROPERTY
    vtkPVVTKExtensionsCGNSReader_SERVERMANAGER_XMLS
    ${CMAKE_CURRENT_LIST_DIR}/resources/CGNSReader.xml
)
unset(cgns_private_depends)
