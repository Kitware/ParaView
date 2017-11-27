set (__dependencies)
if (PARAVIEW_USE_MPI)
  set (__dependencies
    vtkFiltersParallelMPI
    vtkParallelMPI
    )
  if (PARAVIEW_USE_ICE_T)
    list(APPEND __dependencies vtkicet)
  endif()
endif()

if(PARAVIEW_ENABLE_NVPIPE)
  list(APPEND __dependencies vtknvpipe vtkPVClientServerCoreCore)
endif()

if(PARAVIEW_ENABLE_MATPLOTLIB)
  list(APPEND __dependencies vtkRenderingMatplotlib)
endif()

if(PARAVIEW_USE_OSPRAY)
  list(APPEND __dependencies vtkRenderingOSPRay)
endif()

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
    Qt
    ParaViewRendering
  PRIVATE_DEPENDS
    vtkCommonColor
    vtkglew
  DEPENDS
    vtkChartsCore
    vtkFiltersExtraction
    vtkFiltersGeneric
    vtkFiltersHyperTree
    vtkFiltersParallel
    vtkInteractionStyle
    vtkInteractionWidgets
    vtkIOXML
    vtkPVVTKExtensionsCore
    vtkRenderingAnnotation
    vtkRenderingFreeType
    vtkRenderingOpenGL2
    vtkRenderingContextOpenGL2
    vtkRenderingParallel
    vtkIOExport
    vtkIOExportOpenGL2
    ${__dependencies}
    vtkRenderingVolumeAMR
    vtkCommonComputationalGeometry
    vtkCommonSystem
    vtkIOImage
  PRIVATE_DEPENDS
    vtkzlib
    vtklz4

  TEST_DEPENDS
    vtkInteractionStyle
    vtkIOAMR
    vtkIOXML
    vtkTestingCore
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)
