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
set (Module_SRCS
  vtkPVCatalystSessionCore.cxx
  vtkPVFilePathEncodingHelper.cxx
  vtkPVProxyDefinitionIterator.cxx
  vtkPVSessionBase.cxx
  vtkPVSessionCore.cxx
  vtkPVSessionCoreInterpreterHelper.cxx
  vtkPVSessionServer.cxx
  vtkSIArraySelectionProperty.cxx
  vtkSIIndexSelectionProperty.cxx
  vtkSICollaborationManager.cxx
  vtkSICompoundSourceProxy.cxx
  vtkSIDataArrayProperty.cxx
  vtkSIDirectoryProxy.cxx
  vtkSIDoubleMapProperty.cxx
  vtkSIDoubleVectorProperty.cxx
  vtkSIMetaReaderProxy.cxx
  vtkSIIdTypeVectorProperty.cxx
  vtkSIInputProperty.cxx
  vtkSIIntVectorProperty.cxx
  vtkSIObject.cxx
  vtkSIProperty.cxx
  vtkSIProxy.cxx
  vtkSIProxyDefinitionManager.cxx
  vtkSIProxyProperty.cxx
  vtkSISILProperty.cxx
  vtkSISourceProxy.cxx
  vtkSIStringVectorProperty.cxx
  vtkSITimeLabelProperty.cxx
  vtkSITimeRangeProperty.cxx
  vtkSITimeStepsProperty.cxx
  vtkSIVectorProperty.cxx
  vtkSIWriterProxy.cxx
  vtkSMMessage.cxx
)

set_source_files_properties(
  vtkSMMessage
  WRAP_EXCLUDE)

set_source_files_properties(
  vtkPVProxyDefinitionIterator.cxx
  vtkPVSessionBase.cxx
  vtkSIVectorProperty.cxx
  vtkSMMessage.cxx
  ABSTRACT)

# Add header-only files so they get installed correctly.
set (${vtk-module}_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkSMMessageMinimal.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
  ${CMAKE_CURRENT_SOURCE_DIR}/vtkSIVectorPropertyTemplate.h
  )

#------------------------------------------------------------------------------
# Generate the protbuf message file.
#------------------------------------------------------------------------------
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
           ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc

    COMMAND protoc_compiler
           "--cpp_out=dllexport_decl=VTKPVSERVERIMPLEMENTATIONCORE_EXPORT:${CMAKE_CURRENT_BINARY_DIR}"
           --proto_path "${CMAKE_CURRENT_SOURCE_DIR}"
           "${CMAKE_CURRENT_SOURCE_DIR}/vtkPVMessage.proto"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vtkPVMessage.proto protoc_compiler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
  protobuf_code_generation
  DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc
)

#------------------------------------------------------------------------------
# OpenGL2 specific definitions
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()

vtk_module_library(vtkPVServerImplementationCore
  ${Module_SRCS})

if (VTK_USE_SYSTEM_PROTOBUF)
  # Protobuf 3.1 doesn't decorate some of its symbols with the _EXPORT macro,
  # so they are not visible when using `-fvisibility=hidden`. Explicitly set
  # the visibility to `default` to work around this. It is fixed as of
  # protobuf-3.2.0-rc1 by this commit:
  # <https://github.com/google/protobuf/commit/b1295eeffbf2ae7da8a5c7d8601c3ace0663fa09>.
  set_target_properties(vtkPVServerImplementationCore
    PROPERTIES
      CXX_VISIBILITY_PRESET default)
endif ()

add_dependencies(vtkPVServerImplementationCore
  protobuf_code_generation)
