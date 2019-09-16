set(classes
  vtkAbstractAccumulator
  vtkAbstractArrayMeasurement
  vtkArithmeticAccumulator
  vtkArithmeticMeanArrayMeasurement
  vtkEntropyAccumulator
  vtkEntropyArrayMeasurement
  vtkGeometricAccumulator
  vtkGeometricMeanArrayMeasurement
  vtkHarmonicMeanArrayMeasurement
  vtkResampleToHyperTreeGrid
  vtkInversedArithmeticAccumulator
  vtkMedianAccumulator
  vtkMedianArrayMeasurement
  vtkSquaredArithmeticAccumulator
  vtkStandardDeviationArrayMeasurement)

vtk_module_add_module(HyperTreeGridFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)
