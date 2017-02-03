vtk_module(vtkPVVTKExtensionsCosmoTools
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
   PRIVATE_DEPENDS
      vtkParallelMPI
      vtkPVVTKExtensionsDefault
      vtkjsoncpp
      vtkCosmoHaloFinder
   KIT
      vtkPVExtensions
   TEST_LABELS
     PARAVIEW
   TEST_DEPENDS
      vtkTestingCore
      vtkTestingRendering
      vtkParallelMPI
)

# paraview-specific extensions to a module to bring in proxy XML configs
set_property(GLOBAL PROPERTY
    vtkPVVTKExtensionsCosmoTools_SERVERMANAGER_XMLS

    ## CosmoTools Readers
    ${CMAKE_CURRENT_LIST_DIR}/resources/AdaptiveCosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/CosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/GenericIOReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/GenericIOWriter.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/MultiResolutionGenericIOReader.xml

    ## CosmoTools Filters
    ${CMAKE_CURRENT_LIST_DIR}/resources/LANLHaloFinder.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/ANLHaloFinder.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/ANLSubHaloFinder.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/MergeConnected.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/MinkowskiFilter.xml
    )
