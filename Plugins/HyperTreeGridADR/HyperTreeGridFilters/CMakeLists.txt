set(classes
  vtkAbstractAccumulator
  vtkAbstractArrayMeasurement
  vtkArithmeticMeanArrayMeasurement
  vtkEntropyArrayMeasurement
  vtkGeometricMeanArrayMeasurement
  vtkHarmonicMeanArrayMeasurement
  vtkQuantileAccumulator
  vtkQuantileArrayMeasurement
  vtkResampleToHyperTreeGrid
  vtkStandardDeviationArrayMeasurement)

set(headers
  vtkFunctor.h)

set(template_classes
  vtkArithmeticAccumulator
  vtkBinsAccumulator)

vtk_module_add_module(HyperTreeGridFilters
  CLASSES ${classes}
  HEADERS ${headers}
  TEMPLATE_CLASSES ${template_classes})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)
