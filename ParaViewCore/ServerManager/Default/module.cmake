# If FFMPEG support is enabled, we need to depend on FFMPEG.
set (__extra_dependencies)
if (PARAVIEW_ENABLE_FFMPEG)
  list(APPEND __extra_dependencies vtkIOFFMPEG)
endif()

vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkIOMovie
    vtkIOParallelExodus
    vtkPVServerImplementationDefault
    vtkPVServerManagerRendering
    vtkRenderingVolumeOpenGL
    vtkTestingRendering
    ${__extra_dependencies}
  PRIVATE_DEPENDS
    vtksys
  TEST_DEPENDS
    vtkPVServerManagerApplication
  TEST_LABELS
    PARAVIEW
)
unset(__extra_dependencies)

# Add XML resources.
set_property(GLOBAL PROPERTY
  vtkPVServerManagerDefault_SERVERMANAGER_XMLS
  ${CMAKE_CURRENT_LIST_DIR}/options.xml)
