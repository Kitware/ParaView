set (_dependencies)
if (PARAVIEW_USE_MPI)
  list(APPEND _dependencies vtkIOMPIImage vtkFiltersParallelFlowPaths)
elseif()
  list(APPEND _dependencies vtkIOImage)
endif()

vtk_module(vtkPVVTKExtensionsDefault
  DEPENDS
    vtkFiltersAMR
    vtkFiltersExtraction
    vtkFiltersGeneral
    vtkFiltersParallelStatistics
    vtkImagingFourier
    vtkImagingSources
    vtkInteractionWidgets
    vtkIOEnSight
    vtkIOImport
    vtkIOParallelExodus
    vtkPVVTKExtensionsCore
    vtkPVVTKExtensionsRendering
    vtkIOParallel
    vtkIOParallelXML
    ${_dependencies}
  PRIVATE_DEPENDS
    vtkIOInfovis
    vtknetcdfcpp
    vtksys
    vtkChartsCore
    vtkIOPLY
  TEST_DEPENDS
    vtkTestingCore
  TEST_LABELS
    PARAVIEW
  KIT
    vtkPVExtensions
)
