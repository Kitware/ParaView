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

# Configure protobuf
set (PROTOBUF_INSTALL_BIN_DIR ${VTK_INSTALL_RUNTIME_DIR})
set (PROTOBUF_INSTALL_LIB_DIR ${VTK_INSTALL_LIBRARY_DIR})
set (PROTOBUF_INSTALL_EXPORT_NAME ${VTK_INSTALL_EXPORT_NAME})

# we don't use zlib compression
set (PROTOBUF_ENABLE_ZLIB 0)

vtk_module_third_party(Protobuf
  INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/vtkprotobuf/src
  LIBRARIES protobuf
)

# protobuf exports it's build-dir targets to a custom file
# (PROTOBUF_EXPORTS.cmake). We don't care much about that. We export
# build-dir targets ourselves.
if(NOT VTK_USE_SYSTEM_PROTOBUF)
  vtk_target_export(protobuf)
  vtk_target_export(protobuf-lite)
  if (NOT CMAKE_CROSSCOMPILING)
    vtk_compile_tools_target_export(protoc_compiler)
  endif()
else()
  add_executable(protoc_compiler IMPORTED GLOBAL)
  set_target_properties(protoc_compiler PROPERTIES
    IMPORTED_LOCATION "${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

# All these exports don't add any install rules. However we make protobuf itself
# install components at the right location by setting PROTOBUF_INSTALL_*
# variables.
