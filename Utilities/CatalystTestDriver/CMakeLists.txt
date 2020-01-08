set(classes
  vtkCPBaseFieldBuilder
  vtkCPBaseGridBuilder
  vtkCPCellFieldBuilder
  vtkCPConstantScalarFieldFunction
  vtkCPFieldBuilder
  vtkCPFileGridBuilder
  vtkCPGridBuilder
  vtkCPLinearScalarFieldFunction
  vtkCPMultiBlockGridBuilder
  vtkCPNodalFieldBuilder
  vtkCPScalarFieldFunction
  vtkCPTensorFieldFunction
  vtkCPTestDriver
  vtkCPUniformGridBuilder
  vtkCPUnstructuredGridBuilder
  vtkCPVector3FieldFunction)

vtk_module_add_module(ParaView::CatalystTestDriver
  CLASSES ${classes})
vtk_module_client_server_exclude()
