#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================

# FIXME: export symbols on non-Windows as well.
set(CMAKE_CXX_VISIBILITY_PRESET "default")
set(CMAKE_VISIBILITY_INLINES_HIDDEN 0)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkprotobuf/LICENSE"
    VERSION       "3.13.0"
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
