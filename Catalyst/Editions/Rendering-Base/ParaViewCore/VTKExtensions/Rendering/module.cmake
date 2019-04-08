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

list(APPEND __dependencies vtkglew)

if(PARAVIEW_USE_RAYTRACING)
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
    vtkIOExportOpenGL2
    vtkIOImage
    vtkIOXML
    vtkInteractionStyle
    vtkParallelMPI
    vtkRenderingAnnotation
    vtkRenderingOpenGL2
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
