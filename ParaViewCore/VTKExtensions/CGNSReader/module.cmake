vtk_module(vtkPVVTKExtensionsCGNSReader
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
      vtkPVVTKExtensionsCore
    PRIVATE_DEPENDS
      vtkhdf5
      vtkcgns
      vtksys
      vtkParallelCore
      vtkFiltersExtraction
      vtkPVVTKExtensionsSIL
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
