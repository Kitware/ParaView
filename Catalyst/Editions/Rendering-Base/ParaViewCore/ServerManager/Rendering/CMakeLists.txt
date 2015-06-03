set(Module_SRCS)

file(GLOB cmake_files "${CMAKE_CURRENT_SOURCE_DIR}/*.catalyst.cmake")

foreach (cmake_file IN LISTS cmake_files)
  include("${cmake_file}")
endforeach ()

#-----------------------------------------------------------------------------
# Stuff for ObjectFactory override for vtkSMParaViewPipelineControllerWithRendering.
list(APPEND Module_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Override some rendering classes. Create object factory:
set(vtk_module_overrides "vtkSMParaViewPipelineController")
set(vtk_module_vtkSMParaViewPipelineController_override
  "vtkSMParaViewPipelineControllerWithRendering")

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Compile ColorMaps.json
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json
          kwProcessXML
  COMMAND kwProcessXML
          ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
          vtkSMTransferFunctionPresets JSON JSON
          ${CMAKE_CURRENT_SOURCE_DIR}/ColorMaps.json
  COMMENT "Processing builtin color maps")
set(${vtk-module}_HDRS
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/vtkSMTransferFunctionPresetsBuiltin.h
  HEADER_FILE_ONLY
  WRAP_EXCLUDE)
#-----------------------------------------------------------------------------

vtk_module_library(vtkPVServerManagerRendering ${Module_SRCS})
