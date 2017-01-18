set (__dependencies)
if (PARAVIEW_USE_MPI)
    #set (__dependencies vtkFiltersParallelMPI)
  if (PARAVIEW_USE_ICE_T)
      #list(APPEND __dependencies vtkicet)
  endif()

  # needed for mpich
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
endif()

if(PARAVIEW_ENABLE_PYTHON)
  #list(APPEND __dependencies vtkRenderingMatplotlib)
endif()

if (PARAVIEW_ENABLE_QT_SUPPORT)
  list(APPEND __dependencies vtkGUISupportQt)
endif()
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
  #list(APPEND __dependencies vtkRenderingLIC)
  if (PARAVIEW_USE_MPI)
    #list (APPEND __dependencies vtkRenderingParallelLIC)
  endif()
else()
  list(APPEND __dependencies vtkglew)
endif()

if(PARAVIEW_USE_OSPRAY)
  #list(APPEND __dependencies vtkRenderingOSPRay)
endif()

if (Module_vtkRenderingCore)
  list(APPEND __dependencies
    vtkChartsCore
    vtkCommonColor
    vtkCommonComputationalGeometry
    vtkFiltersExtraction
    vtkFiltersGeneric
    vtkFiltersHyperTree
    vtkFiltersParallel
    vtkFiltersParallelMPI
    vtkIOExport
    vtkIOExport${VTK_RENDERING_BACKEND}
    vtkIOImage
    vtkIOXML
    vtkInteractionStyle
    vtkParallelMPI
    vtkRenderingAnnotation
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingParallel
    vtkicet
    vtklz4)
endif ()

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
    Qt
    ParaViewRendering
  DEPENDS
    vtkFiltersExtraction
    vtkFiltersSources
    vtkPVVTKExtensionsCore

    ${__dependencies}
  COMPILE_DEPENDS

  TEST_DEPENDS
    vtkInteractionStyle
    vtkIOAMR
    vtkIOXML
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)
