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
  vtkPVProxyDefinitionIterator.cxx
  vtkPVSessionBase.cxx
  vtkPVSessionCore.cxx
  vtkPVSessionCoreInterpreterHelper.cxx
  vtkPVSessionServer.cxx
  vtkSIAMRVolumeRepresentationProxy.cxx
  vtkSIArraySelectionProperty.cxx
  vtkSIChartRepresentationProxy.cxx
  vtkSICollaborationManager.cxx
  vtkSICompoundSourceProxy.cxx
  vtkSIContextArraysProperty.cxx
  vtkSIDataArrayProperty.cxx
  vtkSIDoubleVectorProperty.cxx
  vtkSIFileSeriesReaderProxy.cxx
  vtkSIIdTypeVectorProperty.cxx
  vtkSIImageTextureProxy.cxx
  vtkSIInputProperty.cxx
  vtkSIIntVectorProperty.cxx
  vtkSIObject.cxx
  vtkSIProperty.cxx
  vtkSIProxy.cxx
  vtkSIProxyDefinitionManager.cxx
  vtkSIProxyProperty.cxx
  vtkSIPVRepresentationProxy.cxx
  vtkSIScalarBarActorProxy.cxx
  vtkSISelectionRepresentationProxy.cxx
  vtkSISILProperty.cxx
  vtkSISourceProxy.cxx
  vtkSIStringVectorProperty.cxx
  vtkSITextSourceRepresentationProxy.cxx
  vtkSITimeRangeProperty.cxx
  vtkSITimeStepsProperty.cxx
  vtkSIUniformGridVolumeRepresentationProxy.cxx
  vtkSIUnstructuredGridVolumeRepresentationProxy.cxx
  vtkSIVectorProperty.cxx
  vtkSIWriterProxy.cxx
  vtkSIXMLAnimationWriterRepresentationProperty.cxx
  vtkSMMessage.cxx
)

set_source_files_properties(
  vtkSMMessage
  WRAP_EXCLUDE)

set_source_files_properties(
  vtkPVProxyDefinitionIterator.cxx
  vtkSIVectorProperty.cxx
  vtkPVSessionBase.cxx
  vtkSMMessage.cxx
  ABSTRACT)

#------------------------------------------------------------------------------
# Generate the protbuf message file.
#------------------------------------------------------------------------------
get_target_property(PROTOC_LOCATION protoc_compiler LOCATION)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
           ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc
    
    COMMAND protoc_compiler
           "--cpp_out=dllexport_decl=VTK_PROTOBUF_EXPORT:${CMAKE_CURRENT_BINARY_DIR}"
           --proto_path "@CMAKE_CURRENT_SOURCE_DIR@/Resources"
           "${CMAKE_CURRENT_SOURCE_DIR}/Resources/vtkPVMessage.proto"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/vtkPVMessage.proto protoc_compiler
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(
  protobuf_code_generation
  DEPENDS 
    ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtkPVMessage.pb.cc
)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Generate header from the xml files.
#------------------------------------------------------------------------------
set (resourceFiles
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/filters.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/sources.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/readers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/utilities.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/rendering.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/views_and_representations.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/3d_widgets.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/internal_writers.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/Resources/writers.xml
  )

cmake_dependent_option(PARAVIEW_ENABLE_PYTHON_FILTERS
  "Enable Python calculator and programmable filters." ON
  "PARAVIEW_ENABLE_PYTHON" OFF)

if (PARAVIEW_ENABLE_PYTHON_FILTERS)
  list(APPEND resourceFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pythonfilter.xml)
endif()

set (PARAVIEW_SERVERMANAGER_XMLS ${resourceFiles}
  CACHE INTERNAL "Server Manager XMLs")

#------------------------------------------------------------------------------


vtk_module_library(vtkPVServerImplementation
  ${Module_SRCS})

add_dependencies(vtkPVServerImplementation
  protobuf_code_generation)
