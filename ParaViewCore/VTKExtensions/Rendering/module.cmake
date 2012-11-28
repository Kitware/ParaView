set (__dependencies)
if (PARAVIEW_USE_MPI)
  set (__dependencies vtkFiltersParallelMPI)
  if (PARAVIEW_USE_ICE_T)
    list(APPEND __dependencies vtkicet)
  endif()

  # needed for mpich
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
endif()

if (PARAVIEW_ENABLE_QT_SUPPORT)
    # This is bad to bring Qt dependency here...
    # this should be clean up for 4.0
    list(APPEND __dependencies vtkGUISupportQt)
    find_package(Qt4)
    include(${QT_USE_FILE})
endif()

vtk_module(vtkPVVTKExtensionsRendering
  GROUPS
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

    ${__dependencies}
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString

  TEST_DEPENDS
    vtkInteractionStyle
    vtkIOAMR
    vtkIOXML
    vtkRenderingOpenGL
    vtkTestingRendering

  TEST_LABELS
    PARAVIEW
)
