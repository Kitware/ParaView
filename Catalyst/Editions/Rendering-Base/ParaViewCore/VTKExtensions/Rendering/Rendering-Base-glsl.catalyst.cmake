# Encode glsl files.
foreach (file vtkIceTCompositeZPassShader_fs.glsl)
  get_filename_component(file_we ${file} NAME_WE)
  vtk_encode_string(
    INPUT         "${file}"
    NAME          "${file_we}"
    EXPORT_SYMBOL "VTKPVVTKEXTENSIONSRENDERING_EXPORT"
    EXPORT_HEADER "vtkPVVTKExtensionsRenderingModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND Module_SRCS ${source})
endforeach()
