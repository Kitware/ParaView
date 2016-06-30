vtk_module(vtkPVCinemaReader
  DEPENDS
    vtkCommonExecutionModel
    vtkPVServerManagerCore
    vtkRenderingCore

  PRIVATE_DEPENDS
    CinemaPython
    vtkPVAnimation
    vtkPVClientServerCoreRendering
    vtkPVServerManagerRendering
    vtkPythonInterpreter
    vtkRenderingOpenGL2

  TEST_LABELS
    PARAVIEW
)
set_property(GLOBAL PROPERTY
  vtkPVCinemaReader_SERVERMANAGER_XMLS ${CMAKE_CURRENT_LIST_DIR}/cinemareader.xml)
