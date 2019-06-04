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
  vtkHyperTreeGridBox
  vtkHyperTreeGridPlane
  vtkInversedArithmeticAccumulator
  vtkMedianAccumulator
  vtkMedianArrayMeasurement
  vtkSquaredArithmeticAccumulator
  vtkStandardDeviationArrayMeasurement)

vtk_module_add_module(HyperTreeGridFilters
  CLASSES ${classes})

paraview_add_server_manager_xmls(
  XMLS  HyperTreeGridFilters.xml)

#vtk_module_wrap_python(
#  MODULES HyperTreeGridFilters
#  INSTALL_HEADERS OFF
#  PYTHON_PACKAGE paraview.simple
#)
