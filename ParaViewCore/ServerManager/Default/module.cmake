vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerManagerRendering
  PRIVATE_DEPENDS
    vtksys
    vtkRenderingVolumeOpenGL
    vtkTestingRendering
    vtkPVClientServerCoreDefault
  TEST_DEPENDS
    vtkPVServerManagerApplication
    vtkTestingCore
  TEST_LABELS
    PARAVIEW
)

# Add XML resources.
set_property(GLOBAL PROPERTY
  vtkPVServerManagerDefault_SERVERMANAGER_XMLS
  ${CMAKE_CURRENT_LIST_DIR}/settings.xml)
