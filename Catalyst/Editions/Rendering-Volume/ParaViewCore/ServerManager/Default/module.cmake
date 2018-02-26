# If FFMPEG support is enabled, we need to depend on FFMPEG.
set (__extra_dependencies)
if (PARAVIEW_ENABLE_FFMPEG)
  list(APPEND __extra_dependencies vtkIOFFMPEG)
endif()

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    #vtkIOMovie
    #vtkIOParallelExodus
    #vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    #vtkRenderingVolumeOpenGL
    #vtkTestingRendering
    ${__extra_dependencies}
  PRIVATE_DEPENDS
    vtksys
  TEST_DEPENDS
    vtkPVServerManagerApplication
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVServerManager
)
unset(__extra_dependencies)
