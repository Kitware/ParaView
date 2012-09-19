set (extra_depends)
if (PARAVIEW_USE_MPI)
  list(APPEND extra_depends vtkIOMPIImage
    )
endif()

if (Module_vtkIOFFMPEG)
  list(APPEND extra_depends vtkIOFFMPEG)
endif()

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkIOMovie
    vtkIOParallelExodus
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    vtkRenderingVolumeOpenGL
    vtkTestingRendering
    ${extra_depends}
  TEST_DEPENDS
    vtkPVServerManagerApplication
)
unset(extra_depends)
