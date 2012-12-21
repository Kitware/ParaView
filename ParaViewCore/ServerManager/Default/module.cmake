set (extra_depends)
if (PARAVIEW_USE_MPI)
  list(APPEND extra_depends vtkIOMPIImage
    )
endif()

if (PARAVIEW_USE_MATPLOTLIB)
  list(APPEND extra_depends vtkRenderingMatplotlib)
endif()

if (PARAVIEW_ENABLE_FFMPEG)
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
  TEST_LABELS
    PARAVIEW
)
unset(extra_depends)
