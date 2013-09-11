vtk_module(vtkPVVTKExtensionsCosmoTools
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
   PRIVATE_DEPENDS
      vtkParallelMPI
)

# paraview-specific extensions to a module to bring in proxy XML configs
set_property(GLOBAL PROPERTY
    vtkPVVTKExtensionsCosmoTools_SERVERMANAGER_XMLS

    ## CosmoTools Readers
    ${CMAKE_CURRENT_LIST_DIR}/resources/CosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/AdaptiveCosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/GenericIOReader.xml

    ## CosmoTools Filters
    ${CMAKE_CURRENT_LIST_DIR}/resources/LANLHaloFinder.xml
    )