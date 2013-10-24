list(APPEND Module_SRCS
  vtkAMRDualClip.cxx
  vtkAMRDualContour.cxx
  vtkAMRDualGridHelper.cxx
  vtkPVArrayCalculator.cxx
  vtkPVBox.cxx
  vtkPVClipDataSet.cxx
  vtkPVContourFilter.cxx
  vtkPVDataSetAlgorithmSelectorFilter.cxx
  vtkPVGlyphFilter.cxx
  vtkPVMetaClipDataSet.cxx
  vtkPVMetaSliceDataSet.cxx
  vtkPVPlane.cxx
)

set_source_files_properties(
  vtkAMRDualGridHelper
  WRAP_EXCLUDE
  )
