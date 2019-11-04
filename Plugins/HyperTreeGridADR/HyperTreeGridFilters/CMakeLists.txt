set(classes
  vtkAbstractAccumulator
  vtkAbstractArrayMeasurement
  vtkArithmeticAccumulator
  vtkArithmeticMeanArrayMeasurement
  vtkBinsAccumulator
  vtkEntropyArrayMeasurement
  vtkGeometricMeanArrayMeasurement
  vtkHarmonicMeanArrayMeasurement
  vtkQuantileAccumulator
  vtkQuantileArrayMeasurement
  vtkFunctionOfXList
  vtkResampleToHyperTreeGrid
  vtkStandardDeviationArrayMeasurement)

vtk_module_add_module(HyperTreeGridFilters
  CLASSES ${classes}
  HEADERS ${headers})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)
