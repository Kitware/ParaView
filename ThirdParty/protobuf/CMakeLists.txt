# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkprotobuf/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2008 Google Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/protobuf.git@for/paraview-20220208-3.13.0"
    VERSION
      "3.13.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Protobuf
    VERSION "3.4"
    TARGETS protobuf::libprotobuf
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_ParaView_protobuf)
  # promote protobuf::protoc to GLOBAL to allow to call protobuf_generate from other directories
  set_target_properties (protobuf::protoc PROPERTIES IMPORTED_GLOBAL TRUE)
  set(protobuf_function_file "${CMAKE_CURRENT_SOURCE_DIR}/protobuf-function.cmake")
else ()
  set(protobuf_function_file "${CMAKE_CURRENT_SOURCE_DIR}/vtkprotobuf/cmake/protobuf-function.cmake")
endif ()
set(protobuf_function_file_output
  "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_build_CMAKE_DESTINATION}/protobuf-function.cmake")
configure_file(
  "${protobuf_function_file}"
  "${protobuf_function_file_output}"
  COPYONLY)
include("${protobuf_function_file_output}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_protobuf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_protobuf.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_protobuf.h")
