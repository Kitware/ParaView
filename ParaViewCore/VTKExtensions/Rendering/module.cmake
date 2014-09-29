set (__dependencies)
if (PARAVIEW_USE_MPI)
  set (__dependencies
    vtkFiltersParallelMPI
    vtkRenderingParallelLIC
    )
  if (PARAVIEW_USE_ICE_T)
    list(APPEND __dependencies vtkicet)
  endif()
endif()

if(PARAVIEW_ENABLE_MATPLOTLIB)
  list(APPEND __dependencies vtkRenderingMatplotlib)
endif()

if (PARAVIEW_ENABLE_QT_SUPPORT)
  list(APPEND __dependencies vtkGUISupportQt)
endif(PARAVIEW_ENABLE_QT_SUPPORT)

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
    Qt
    ParaViewRendering
  PRIVATE_DEPENDS
    vtkCommonColor
  DEPENDS
    vtkChartsCore
    vtkFiltersExtraction
    vtkFiltersGeneric
    vtkFiltersHyperTree
    vtkFiltersParallel
    vtkInteractionStyle
    vtkInteractionWidgets
#    vtkIOExport
    vtkIOXML
    vtkPVVTKExtensionsCore
    vtkRenderingAnnotation
    vtkRenderingFreeType${VTK_RENDERING_BACKEND}
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingParallel
#    vtkRenderingLIC

    ${__dependencies}
PRIVATE_DEPENDS
    vtkzlib
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString

  TEST_DEPENDS
    vtkInteractionStyle
    vtkIOAMR
    vtkIOXML
#    vtkRenderingLIC
    vtkTestingCore
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)
