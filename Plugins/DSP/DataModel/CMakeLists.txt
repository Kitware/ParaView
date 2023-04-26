set(headers
  vtkArrayDispatchDSPArrayList.h
  vtkMultiDimensionalArray.h
  vtkMultiDimensionalImplicitBackend.h
)

set(private_classes
  vtkDSPIterator
  vtkDSPMultiBlockIterator
  vtkDSPTableIterator
)

vtk_module_add_module(DigitalSignalProcessing::DSPDataModel
  FORCE_STATIC  # build static library, to avoid confusion when loading
  NOWRAP_HEADERS  ${headers}
  PRIVATE_CLASSES ${private_classes}
)
