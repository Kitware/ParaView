# Module for ParaView Web.

vtk_module(vtkParaViewWeb
  DEPENDS
    vtkPVServerManagerDefault
  TEST_DEPENDS
    vtkImagingSources
  TEST_LABELS
    PARAVIEW
    PARAVIEWWEB
)
