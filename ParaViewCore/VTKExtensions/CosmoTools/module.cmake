vtk_module(vtkPVVTKExtensionsCosmoTools
    DEPENDS
      vtkCommonCore
      vtkCommonDataModel
      vtkCommonExecutionModel
      vtkParallelCore
   PRIVATE_DEPENDS
      vtkParallelMPI
   KIT
      vtkPVExtensions
)

# paraview-specific extensions to a module to bring in proxy XML configs
set_property(GLOBAL PROPERTY
    vtkPVVTKExtensionsCosmoTools_SERVERMANAGER_XMLS

    ## CosmoTools Readers
    ${CMAKE_CURRENT_LIST_DIR}/resources/AdaptiveCosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/CosmoReader.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/GenericIOReader.xml

    ## CosmoTools Filters
    ${CMAKE_CURRENT_LIST_DIR}/resources/LANLHaloFinder.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/MergeConnected.xml
    ${CMAKE_CURRENT_LIST_DIR}/resources/MinkowskiFilter.xml
    )

if (${COSMOTOOLS_HAS_VORONOI})
  set_property(GLOBAL APPEND PROPERTY
    vtkPVVTKExtensionsCosmoTools_SERVERMANAGER_XMLS
    ${CMAKE_CURRENT_LIST_DIR}/resources/VoronoiReader.xml)
endif()
