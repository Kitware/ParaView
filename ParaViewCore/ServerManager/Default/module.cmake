set (extra_depends)
if (PARAVIEW_USE_MPI)
  list(APPEND extra_depends
    vtkIOMPIImage)
endif()

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    vtkTestingRendering
    ${extra_depends}
  TEST_DEPENDS
    vtkPVServerManagerApplication
)
unset(extra_depends)
