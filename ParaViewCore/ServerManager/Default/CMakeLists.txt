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
  vtkLoadStateOptions.cxx
  vtkPVGeneralSettings.cxx
  vtkPVMultiServerDataSource.cxx
  vtkSMCameraConfigurationReader.cxx
  vtkSMCameraConfigurationWriter.cxx
  vtkSMCSVExporterProxy.cxx
  vtkSMDirectoryProxy.cxx
  vtkSMEnsembleDataReaderProxy.cxx
  vtkSMExporterProxy.cxx
  vtkSMGL2PSExporterProxy.cxx
  vtkSMInteractionUndoStackBuilder.cxx
  vtkSMInteractiveSelectionPipeline.cxx
  vtkSMLoadStateOptionsProxy.cxx
  vtkSMMultiServerSourceProxy.cxx
  vtkSMPLYWriterProxyInitializationHelper.cxx
  vtkSMPreselectionPipeline.cxx
  vtkSMPropertyModificationUndoElement.cxx
  vtkSMProxyClipboard.cxx
  vtkSMProxyConfigurationReader.cxx
  vtkSMProxyConfigurationWriter.cxx
  vtkSMPSWriterProxy.cxx
  vtkSMReaderReloadHelper.cxx
  vtkSMRenderViewExporterProxy.cxx
  vtkSMTesting.cxx
  vtkSMTooltipSelectionPipeline.cxx
  vtkSMViewExportHelper.cxx
)

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()

SET_SOURCE_FILES_PROPERTIES(
  vtkSMExporterProxy.cxx
  ABSTRACT
  )

vtk_module_library(vtkPVServerManagerDefault ${Module_SRCS})
