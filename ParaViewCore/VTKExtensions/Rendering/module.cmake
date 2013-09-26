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

if(PARAVIEW_ENABLE_PYTHON AND PARAVIEW_ENABLE_MATPLOTLIB)
  list(APPEND __dependencies vtkRenderingMatplotlib)
endif()

if (PARAVIEW_ENABLE_QT_SUPPORT)
  list(APPEND __dependencies vtkGUISupportQt)
endif(PARAVIEW_ENABLE_QT_SUPPORT)

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
    Qt
    ParaViewRendering
  DEPENDS
    vtkChartsCore
    vtkFiltersExtraction
    vtkFiltersGeneric
    vtkFiltersHyperTree
    vtkFiltersParallel
    vtkInteractionStyle
    vtkInteractionWidgets
    vtkIOExport
    vtkIOXML
    vtkPVVTKExtensionsCore
    vtkRenderingAnnotation
    vtkRenderingFreeTypeOpenGL
    vtkRenderingOpenGL
    vtkRenderingParallel
    vtkRenderingLIC

    ${__dependencies}
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString

  TEST_DEPENDS
    vtkInteractionStyle
    vtkIOAMR
    vtkIOXML
    vtkRenderingOpenGL
    vtkRenderingLIC
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
)
