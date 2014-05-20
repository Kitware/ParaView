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
  vtkPVProxyDefinitionIterator.cxx
  vtkPVSessionBase.cxx
  vtkPVSessionCore.cxx
  vtkPVSessionCoreInterpreterHelper.cxx
  vtkPVSessionServer.cxx
  vtkSIArraySelectionProperty.cxx
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
if (WIN32 AND BUILD_SHARED_LIBS)
  add_definitions(-DPROTOBUF_USE_DLLS)
endif()

vtk_module_library(vtkPVServerImplementationCore
  ${Module_SRCS})

add_dependencies(vtkPVServerImplementationCore
  protobuf_code_generation)
