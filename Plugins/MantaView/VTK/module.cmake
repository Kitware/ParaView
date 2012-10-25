set (_vtk_modules)
if(PARAVIEW_USE_MPI)
  list(APPEND _vtk_modules vtkParallelMPI)
endif()

if (BUILD_AGAINST_PARAVIEW)
  list(APPEND _vtk_modules vtkPVClientServerCoreRendering)
endif()

vtk_module(vtkManta
  DEPENDS
    vtkRenderingOpenGL
    vtkFiltersCore
    vtkParallelCore
    vtkFiltersHybrid
    ${_vtk_modules}
  TEST_DEPENDS
    vtkTestingRendering
    vtkIOPLY
  TEST_LABELS
    PARAVIEW
  EXCLUDE_FROM_WRAP_HIERARCHY
)
