set (__dependencies)
if (PARAVIEW_USE_MPI)
  set (__dependencies
    vtkFiltersParallelMPI
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
  list(APPEND __dependencies vtkRenderingLIC)
  if (PARAVIEW_USE_MPI)
    list (APPEND __dependencies vtkRenderingParallelLIC)
  endif()
else()
    set(opengl2_private_depends vtkglew)
endif()

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
    Qt
    ParaViewRendering
  PRIVATE_DEPENDS
    vtkCommonColor
    ${opengl2_private_depends}
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
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingParallel
    vtkIOExport
    ${__dependencies}
    vtkRenderingVolumeAMR
  PRIVATE_DEPENDS
    vtkzlib
    vtklz4
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
