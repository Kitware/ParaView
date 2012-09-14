set (extra_depends)
if (PARAVIEW_USE_MPI)
  list(APPEND extra_depends vtkIOMPIImage)
endif()

if (PARAVIEW_ENABLE_FFMPEG)
  list(APPEND extra_depends vtkIOFFMPEG)
endif()

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    vtkTestingRendering
    vtkIOMovie
    ${extra_depends}
  TEST_DEPENDS
    vtkPVServerManagerApplication
)
unset(extra_depends)
