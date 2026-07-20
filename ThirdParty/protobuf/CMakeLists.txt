# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

#
# Finding protobuf in MODULE_MODE doesn't provide information about
# dependencies (eg abseil), and as a result static linking fails when
# an external protobuf is found that way. However, we can't drop support
# for module mode until the fedora system protobuf package provides
# cmke config information. See issue:
#
# https://bugzilla.redhat.com/show_bug.cgi?id=2496122
#
# So we have to continue to support MODULE_MODE for external protobuf
# until that is fixed. At the same time, we must have CONFIG_MODE for
# static builds to avoid linking failures. (every transitive dependency
# lib needs to be listed on the command line)
#

set(protobuf_config_arg)
if (NOT BUILD_SHARED_LIBS)
  set(protobuf_config_arg "CONFIG_MODE")
endif ()

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkprotobuf/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2008 Google Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/protobuf.git@for/paraview-20260206-3.13.0"
    VERSION
      "3.13.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Protobuf
    # Omit external VERSION specifier because protobuf seems to require
    # major version equality, and ParaView just needs anything after 3.4.
    # But we'll try to do our own version check below.
    ${protobuf_config_arg}
    TARGETS protobuf::libprotobuf
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_ParaView_protobuf)
  # We just need to make sure the version found is 3.4 or later. Since the
  # vtk_module_third_party() function seems to swallow any variables set by
  # the internal find_package, we first have to discover what version was found.
  # Of the various options for that, executing "protoc --version" and parsing
  # output seems possibly the most robust, although not concise. If we can't
  # run protoc or we fail to parse the output, just warn that we can't guarantee
  # a suitable version was found.

  # If we found protobuf in config mode, use the target to get the protoc location
  if (NOT Protobuf_PROTOC_EXECUTABLE AND TARGET protobuf::protoc)
    get_target_property(Protobuf_PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION)
    if (NOT Protobuf_PROTOC_EXECUTABLE)
      get_target_property(Protobuf_PROTOC_EXECUTABLE protobuf::protoc IMPORTED_LOCATION_RELEASE)
    endif()
  endif()

  if (Protobuf_PROTOC_EXECUTABLE)
    execute_process(
      COMMAND "${Protobuf_PROTOC_EXECUTABLE}" --version
      ERROR_QUIET
      OUTPUT_VARIABLE _protoc_version_output
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _protoc_result
    )

    # Output typically looks like "libprotoc 3.19.6" or "libprotoc 24.1.0", but could
    # also produce only two digits, in which case those should correspond to the first
    # two digits of a three-digit version (eg 4.24.1, instead of 24.1.0)
    if (_protoc_result EQUAL 0 AND _protoc_version_output MATCHES "([0-9]+\\.[0-9]+(\\.[0-9]+)?)")
      set(extracted_pb_version "${CMAKE_MATCH_1}")
      if (extracted_pb_version VERSION_LESS "3.4")
        message(FATAL_ERROR "ParaView requires Protobuf version 3.4 or newer. Found version: ${extracted_pb_version}")
      endif ()
    else ()
      message(WARNING "Could not parse Protobuf version from protoc output: ${_protoc_version_output}")
    endif ()

  else ()
    message(WARNING "Protobuf_PROTOC_EXECUTABLE not found. Cannot verify Protobuf >= 3.4.")
  endif ()
endif ()

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
