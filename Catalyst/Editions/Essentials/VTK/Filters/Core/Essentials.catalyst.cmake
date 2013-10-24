list(APPEND Module_SRCS
vtkArrayCalculator.cxx
vtkContourFilter.cxx
vtkContourGrid.cxx
vtkContourHelper.cxx
vtkCutter.cxx
vtkGlyph3D.cxx
vtkGridSynchronizedTemplates3D.cxx
vtkMaskPoints.cxx
vtkPolyDataNormals.cxx
vtkRectilinearSynchronizedTemplates.cxx
vtkSynchronizedTemplates2D.cxx
vtkSynchronizedTemplates3D.cxx
vtkSynchronizedTemplatesCutter3D.cxx
)

set_source_files_properties(
  vtkContourHelper
  WRAP_EXCLUDE
  )
