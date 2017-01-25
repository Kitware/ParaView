set (_wanted_dependencies
  vtkIOImage
  vtkFiltersSources
)

set(_all_dependencies)

foreach (_wanted_dependency IN LISTS _wanted_dependencies)
  if (Module_${_wanted_dependency})
    list(APPEND _all_dependencies
      ${_wanted_dependency})
  endif ()
endforeach ()

vtk_module(vtkPVVTKExtensionsCore
  GROUPS
    ParaViewCore
  DEPENDS
    vtkFiltersCore
    vtkParallelCore
    vtkPVCommon
    ${_all_dependencies}
  PRIVATE_DEPENDS
    vtksys
  KIT
    vtkPVExtensions
)
