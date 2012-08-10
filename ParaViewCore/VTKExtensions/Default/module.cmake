#set (_dependencies)
#if (PARAVIEW_USE_MPI)
#  list(APPEND _dependencies
#    vtkFiltersParallelImaging
#    vtkFiltersParallelMPI
##    vtkFiltersParallelTracers
#    vtkIOMPIImage
#    vtkIOParallelNetCDF
#    vtkParallelMPI
#    )
#
#  if (PARAVIEW_USE_ICE_T)
#    list(APPEND _dependencies
#      vtkicet)
#  endif()
#endif()

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
    ${_dependencies}
)
