set (_dependencies)
if (PARAVIEW_USE_MPI)
  list(APPEND _dependencies vtkIOMPIImage)
elseif()
  list(APPEND _dependencies vtkIOImage)
endif()

set(_wanted_dependencies
  vtkFiltersAMR
  vtkFiltersParallelStatistics
  vtkImagingFourier
  vtkImagingSources
  vtkInteractionWidgets
  vtkIOEnSight
  vtkIOImport
  vtkIOParallelExodus
  vtkPVVTKExtensionsRendering
  vtkPVClientServerCoreRendering
  vtkIOParallel
  vtkFiltersGeometry
  ${_dependencies}
  )
set(_all_dependencies)

foreach (_wanted_dependency IN LISTS _wanted_dependencies)
  if (Module_${_wanted_dependency})
    list(APPEND _all_dependencies
      ${_wanted_dependency})
  endif ()
endforeach ()

vtk_module(vtkPVVTKExtensionsDefault
  DEPENDS
    vtkPVVTKExtensionsCore
    vtkFiltersExtraction
    vtkFiltersGeneral
    vtkParallelMPI
    ${_all_dependencies}
  PRIVATE_DEPENDS
    vtksys
  KIT
    vtkPVExtensions
)
