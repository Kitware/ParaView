set(MODULE_HEADERS
  vtkArrayDispatchDSPArrayList.h
  vtkMultiDimensionalArray.h
  vtkMultiDimensionalImplicitBackend.h
)

set(MODULE_PRIVATE_CLASSES
  vtkDSPIterator
  vtkDSPMultiBlockIterator
  vtkDSPTableIterator
)

vtk_module_add_module(DigitalSignalProcessing::DSPDataModel
  FORCE_STATIC  # build static library, to avoid confusion when loading
  NOWRAP_HEADERS  ${MODULE_HEADERS}
  PRIVATE_CLASSES ${MODULE_PRIVATE_CLASSES}
)
