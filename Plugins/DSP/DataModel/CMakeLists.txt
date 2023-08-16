set(MODULE_HEADERS
  vtkArrayDispatchDSPArrayList.h
  vtkMultiDimensionalArray.h
  vtkMultiDimensionalImplicitBackend.h
)

vtk_module_add_module(DigitalSignalProcessing::DSPDataModel
  NOWRAP_HEADERS ${MODULE_HEADERS}
  HEADER_ONLY
)
