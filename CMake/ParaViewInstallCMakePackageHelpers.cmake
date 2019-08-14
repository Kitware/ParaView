function (_paraview_package_append_variables)
  set(_paraview_package_variables)
  foreach (var IN LISTS ARGN)
    if (NOT ${var})
      continue ()
    endif ()

    string(APPEND _paraview_package_variables
      "if (NOT DEFINED \"${var}\")
  set(\"${var}\" \"${${var}}\")
elseif (NOT ${var})
  set(\"${var}\" \"${${var}}\")
endif ()
")
  endforeach ()

  set(paraview_find_package_code
    "${paraview_find_package_code}${_paraview_package_variables}"
    PARENT_SCOPE)
endfunction ()

get_property(_paraview_packages GLOBAL
  PROPERTY _vtk_module_find_packages_ParaView)
if (_paraview_packages)
  list(REMOVE_DUPLICATES _paraview_packages)
endif ()

set(paraview_find_package_code)
foreach (_paraview_package IN LISTS _paraview_packages)
  _paraview_package_append_variables(
    # Standard CMake `find_package` mechanisms.
    "${_paraview_package}_DIR"
    "${_paraview_package}_ROOT"

    # Per-package custom variables.
    ${${_paraview_package}_find_package_vars})
endforeach ()

file(GENERATE
  OUTPUT  "${paraview_cmake_build_dir}/paraview-find-package-helpers.cmake"
  CONTENT "${paraview_find_package_code}")
