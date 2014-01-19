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
  vtkPVComparativeAnimationCue.cxx
  vtkPVComparativeView.cxx
  vtkSMBagChartSeriesListDomain.cxx
  vtkSMBoxRepresentationProxy.cxx
  vtkSMCameraLink.cxx
  vtkSMCameraProxy.cxx
  vtkSMChartRepresentationProxy.cxx
  vtkSMChartSeriesListDomain.cxx
  vtkSMChartSeriesSelectionDomain.cxx
  vtkSMComparativeAnimationCueProxy.cxx
  vtkSMComparativeAnimationCueUndoElement.cxx
  vtkSMComparativeViewProxy.cxx
  vtkSMContextViewProxy.cxx
  vtkSMDataDeliveryManager.cxx
  vtkSMImplicitPlaneRepresentationProxy.cxx
  vtkSMMultiSliceViewProxy.cxx
  vtkSMNewWidgetRepresentationProxy.cxx
  vtkSMPlotMatrixViewProxy.cxx
  vtkSMPVRepresentationProxy.cxx
  vtkSMRenderViewProxy.cxx
  vtkSMRepresentationProxy.cxx
  vtkSMRepresentationTypeDomain.cxx
  vtkSMScalarBarWidgetRepresentationProxy.cxx
  vtkSMSelectionHelper.cxx
  vtkSMSpreadSheetRepresentationProxy.cxx
  vtkSMTextWidgetRepresentationProxy.cxx
  vtkSMTransferFunctionManager.cxx
  vtkSMTransferFunctionProxy.cxx
  vtkSMUtilities.cxx
  vtkSMViewLayoutProxy.cxx
  vtkSMViewProxy.cxx
  vtkSMWidgetRepresentationProxy.cxx
)

if (PARAVIEW_ENABLE_PYTHON AND PARAVIEW_ENABLE_MATPLOTLIB)
  list(APPEND Module_SRCS vtkSMPythonViewProxy.cxx)
endif()

vtk_module_library(vtkPVServerManagerRendering
  ${Module_SRCS})
