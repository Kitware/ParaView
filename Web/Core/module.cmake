# Module for ParaView Web.

vtk_module(vtkParaViewWebCore
  DEPENDS
    vtkWebCore
    vtkPVServerManagerDefault
  TEST_DEPENDS
    vtkImagingSources
  TEST_LABELS
    PARAVIEW
    PARAVIEWWEB
)
