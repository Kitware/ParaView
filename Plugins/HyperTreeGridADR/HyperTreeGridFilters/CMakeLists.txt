set(classes
  vtkAbstractAccumulator
  vtkAbstractArrayMeasurement
  vtkArithmeticMeanArrayMeasurement
  vtkEntropyArrayMeasurement
  vtkFunctor
  vtkGeometricMeanArrayMeasurement
  vtkHarmonicMeanArrayMeasurement
  vtkQuantileAccumulator
  vtkQuantileArrayMeasurement
  vtkResampleToHyperTreeGrid
  vtkStandardDeviationArrayMeasurement)

set(template_classes
  vtkArithmeticAccumulator
  vtkBinsAccumulator)

vtk_module_add_module(HyperTreeGridFilters
  CLASSES ${classes}
  TEMPLATE_CLASSES ${template_classes})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)
