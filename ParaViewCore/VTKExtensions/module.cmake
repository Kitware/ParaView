set (_dependencies)
if (PARAVIEW_USE_MPI)
  list(APPEND _dependencies
    vtkFiltersParallelImaging
    vtkFiltersParallelMPI
#    vtkFiltersParallelTracers
    vtkIOMPIImage
    vtkIOParallelNetCDF
    vtkParallelMPI
    )

  if (PARAVIEW_USE_ICE_T)
    list(APPEND _dependencies
      vtkicet)
  endif()
endif()

vtk_module(vtkPVVTKExtensions
  GROUPS
    ParaView
  DEPENDS
    vtkChartsCore
    vtkFiltersAMR
    vtkFiltersGeneric
    vtkFiltersHyperTree
    vtkFiltersParallelStatistics
    vtkFiltersStatistics
    vtkImagingFourier
    vtkInteractionWidgets
    vtkIOEnSight
    vtkIOImport
    vtkIOParallel
    vtkIOParallelExodus
    vtkPVCommon
    vtkRenderingParallel
    vtkViewsContext2D
    ${_dependencies}

  COMPILE_DEPENDS
    vtkUtilitiesEncodeString
)
