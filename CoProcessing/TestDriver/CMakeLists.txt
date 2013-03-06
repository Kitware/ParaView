SET(TestDriver_SRCS
  vtkCPBaseFieldBuilder.cxx
  vtkCPBaseGridBuilder.cxx
  vtkCPCellFieldBuilder.cxx
  vtkCPConstantScalarFieldFunction.cxx
  vtkCPFieldBuilder.cxx
  vtkCPFileGridBuilder.cxx
  vtkCPGridBuilder.cxx
  vtkCPLinearScalarFieldFunction.cxx
  vtkCPMultiBlockGridBuilder.cxx
  vtkCPNodalFieldBuilder.cxx
  vtkCPScalarFieldFunction.cxx
  vtkCPTensorFieldFunction.cxx
  vtkCPTestDriver.cxx
  vtkCPUniformGridBuilder.cxx
  vtkCPUnstructuredGridBuilder.cxx
  vtkCPVector3FieldFunction.cxx
)

SET_SOURCE_FILES_PROPERTIES(
  vtkCPBaseFieldBuilder
  vtkCPBaseGridBuilder
  vtkCPFieldBuilder
  vtkCPFileGridBuilder
  vtkCPGridBuilder
  vtkCPScalarFieldFunction
  vtkCPTensorFieldFunction
  vtkCPUnstructuredGridBuilder
  vtkCPVector3FieldFunction
  ABSTRACT
)

vtk_module_library(${vtk-module} ${TestDriver_SRCS})
