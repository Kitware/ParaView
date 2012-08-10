set (__dependencies)
if (PARAVIEW_USE_MPI)
  set (__dependencies vtkFiltersParallelMPI)
  if (PARAVIEW_USE_ICE_T)
    list(APPEND __dependencies vtkicet)
  endif()

  # needed for mpich
  add_definitions("-DMPICH_IGNORE_CXX_SEEK")
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
    vtkIOExport
    vtkPVVTKExtensionsCore
    vtkRenderingAnnotation
    vtkRenderingFreeTypeOpenGL
    vtkRenderingParallel
    ${__dependencies}
  COMPILE_DEPENDS
    vtkUtilitiesEncodeString
)
