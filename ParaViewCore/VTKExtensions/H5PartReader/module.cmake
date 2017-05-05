vtk_module(vtkPVVTKExtensionsH5PartReader
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
      vtkPVVTKExtensionsCore
    PRIVATE_DEPENDS
      vtkcgns
      vtkhdf5
      vtksys
      vtkParallelCore
    TEST_DEPENDS
      vtkInteractionStyle
      vtkTestingCore
      vtkTestingRendering
    TEST_LABELS
      PARAVIEW
    KIT
      vtkPVExtensions
    TCL_NAME HFivePartReader
)

set_property(GLOBAL PROPERTY
  vtkPVVTKExtensionsH5PartReader_SERVERMANAGER_XMLS
    ${CMAKE_CURRENT_LIST_DIR}/H5PartServerManager.xml
)
