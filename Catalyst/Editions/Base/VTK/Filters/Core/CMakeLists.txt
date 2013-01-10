set(Module_SRCS
vtkCellDataToPointData.cxx
vtkContourFilter.cxx
vtkContourGrid.cxx
vtkContourHelper.cxx
vtkCutter.cxx
vtkExecutionTimer.cxx
vtkFeatureEdges.cxx
vtkGridSynchronizedTemplates3D.cxx
vtkMarchingCubes.cxx
vtkMarchingSquares.cxx
vtkPointDataToCellData.cxx
vtkPolyDataNormals.cxx
vtkProbeFilter.cxx
vtkQuadricClustering.cxx
vtkRectilinearSynchronizedTemplates.cxx
vtkSynchronizedTemplates2D.cxx
vtkSynchronizedTemplates3D.cxx
vtkSynchronizedTemplatesCutter3D.cxx
vtkThreshold.cxx
vtkAppendCompositeDataLeaves.cxx
vtkAppendFilter.cxx
vtkAppendPolyData.cxx
vtkImageAppend.cxx
)

set_source_files_properties(
  vtkContourHelper
  WRAP_EXCLUDE
  )

vtk_module_library(vtkFiltersCore ${Module_SRCS})
