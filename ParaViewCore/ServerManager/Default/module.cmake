vtk_module(vtkPVServerManagerDefault
  DEPENDS
    vtkPVServerManagerRendering
  PRIVATE_DEPENDS
    vtksys
    vtkpugixml
    vtkRenderingVolumeOpenGL2
    vtkTestingRendering
    vtkPVClientServerCoreDefault
    vtkPVClientServerCoreRendering
  TEST_DEPENDS
    vtkPVServerManagerApplication
    vtkTestingRendering
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVServerManager
)

# Add XML resources.
set_property(GLOBAL PROPERTY
  vtkPVServerManagerDefault_SERVERMANAGER_XMLS
  ${CMAKE_CURRENT_LIST_DIR}/settings.xml)
