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
endif()

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
  list(APPEND __dependencies vtkRenderingLIC vtkIOExport)
endif()

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
    vtkIOXML
    vtkPVVTKExtensionsCore
    vtkRenderingAnnotation
    vtkRenderingFreeType${VTK_RENDERING_BACKEND}
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingParallel

    ${__dependencies}
  PRIVATE_DEPENDS
    vtkzlib
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString

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
