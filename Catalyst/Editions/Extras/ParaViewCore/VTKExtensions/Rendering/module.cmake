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
endif(PARAVIEW_ENABLE_QT_SUPPORT)

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
    vtkIOImage
    vtkIOXML
    vtkInteractionStyle
    vtkParallelMPI
    vtkRenderingAnnotation
    vtkRenderingCore
    vtkRenderingParallel
    vtkicet)
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
    vtkRenderingOpenGL
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)
