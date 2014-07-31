set (_dependencies)
if (PARAVIEW_USE_MPI)
  list(APPEND _dependencies vtkIOMPIImage)
elseif()
  list(APPEND _dependencies vtkIOImage)
endif()

vtk_module(vtkPVVTKExtensionsDefault
  DEPENDS
    vtkFiltersAMR
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
    vtknetcdf
    vtksys
  KIT
    vtkPVExtensions
)
