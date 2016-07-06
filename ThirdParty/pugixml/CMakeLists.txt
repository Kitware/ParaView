vtk_module_third_party(pugixml
  LIBRARIES vtkpugixml
  NO_ADD_SUBDIRECTORY
)
if (VTK_USE_SYSTEM_PUGIXML)
  return()
endif()

set (${vtk-module}_HDRS
     ${CMAKE_CURRENT_SOURCE_DIR}/pugiconfig.h
     ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h)

vtk_add_library(${vtk-module} pugixml.cxx)

#------------------------------------------------------------------------------
# Code copied from vtkModuleMacros.cmake, since vtkModuleMacros doesn't do this
# for vtk_module_third_party().
# Generate the export macro header for symbol visibility/Windows DLL declspec
vtk_generate_export_header(${vtk-module} EXPORT_FILE_NAME ${vtk-module}Module.h)
get_property(_buildtype TARGET ${vtk-module} PROPERTY TYPE)
if (NOT "${_buildtype}" STREQUAL STATIC_LIBRARY)
  # export flags are only added when building shared libs, they cause
  # mismatched visibility warnings when building statically since not all
  # libraries that VTK builds don't set visibility flags. Until we get a
  # time to do that, we skip visibility flags for static libraries.
  if(CMAKE_VERSION VERSION_LESS 3.0)
    #CMake 3.0 deprecates add_compiler_export_flags
    vtk_add_compiler_export_flags(my_abi_flags)
    set_property(TARGET ${vtk-module}${target_suffix} APPEND
      PROPERTY COMPILE_FLAGS "${my_abi_flags}")
  else()
    set_property(TARGET ${vtk-module}${target_suffix}
      PROPERTY CXX_VISIBILITY_PRESET "hidden")
  endif()
endif()
#------------------------------------------------------------------------------
