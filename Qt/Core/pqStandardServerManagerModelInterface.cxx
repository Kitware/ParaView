/*=========================================================================

   Program: ParaView
   Module:    pqStandardServerManagerModelInterface.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqStandardServerManagerModelInterface.h"

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqBoxChartView.h"
#include "pqComparativeRenderView.h"
#include "pqComparativeXYBarChartView.h"
#include "pqComparativeXYChartView.h"
#include "pqExtractor.h"
#include "pqMultiSliceView.h"
#include "pqParallelCoordinatesChartView.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPlotMatrixView.h"
#include "pqRenderView.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqSpreadSheetView.h"
#include "pqTimeKeeper.h"
#include "pqXYBarChartView.h"
#include "pqXYChartView.h"
#include "pqXYHistogramChartView.h"
#include "vtkPVConfig.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSpreadSheetViewProxy.h"

#if PARAVIEW_PQCORE_ENABLE_PYTHON
#include "pqPythonView.h"
#endif

#include <QtDebug>

namespace
{
//-----------------------------------------------------------------------------
inline pqProxy* CreatePQView(
  const QString& group, const QString& name, vtkSMViewProxy* proxy, pqServer* server)
{
  QObject* parent = nullptr;
  QString xmlname = proxy->GetXMLName();
  if (vtkSMSpreadSheetViewProxy::SafeDownCast(proxy))
  {
    return new pqSpreadSheetView(group, name, proxy, server, parent);
  }
  if (xmlname == pqMultiSliceView::multiSliceViewType())
  {
    return new pqMultiSliceView(xmlname, group, name, proxy, server, parent);
  }
#if PARAVIEW_PQCORE_ENABLE_PYTHON
  if (xmlname == pqPythonView::pythonViewType())
  {
    return new pqPythonView(xmlname, group, name, proxy, server, parent);
  }
#endif
  if (vtkSMRenderViewProxy::SafeDownCast(proxy))
  {
    return new pqRenderView(group, name, proxy, server, parent);
  }

  if (vtkSMComparativeViewProxy::SafeDownCast(proxy))
  {
    if (xmlname == pqComparativeXYBarChartView::chartViewType())
    {
      return new pqComparativeXYBarChartView(
        group, name, vtkSMComparativeViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == pqComparativeXYChartView::chartViewType())
    {
      return new pqComparativeXYChartView(
        group, name, vtkSMComparativeViewProxy::SafeDownCast(proxy), server, parent);
    }
    // Handle the other comparative render views.
    return new pqComparativeRenderView(group, name, proxy, server, parent);
  }
  if (vtkSMContextViewProxy::SafeDownCast(proxy))
  {
    if (xmlname == "XYChartView" || xmlname == "XYPointChartView" || xmlname == "QuartileChartView")
    {
      return new pqXYChartView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == "XYBarChartView")
    {
      return new pqXYBarChartView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == "XYHistogramChartView")
    {
      return new pqXYHistogramChartView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == "BoxChartView")
    {
      return new pqBoxChartView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == "ParallelCoordinatesChartView")
    {
      return new pqParallelCoordinatesChartView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    if (xmlname == "PlotMatrixView")
    {
      return new pqPlotMatrixView(
        group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
    }
    // View XML name have not been recognized, default to a pqXYChartView
    return new pqXYChartView(
      group, name, vtkSMContextViewProxy::SafeDownCast(proxy), server, parent);
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
pqStandardServerManagerModelInterface::pqStandardServerManagerModelInterface(QObject* _parent)
  : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqStandardServerManagerModelInterface::~pqStandardServerManagerModelInterface() = default;

//-----------------------------------------------------------------------------
pqProxy* pqStandardServerManagerModelInterface::createPQProxy(
  const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server) const
{
  QString xml_type = proxy->GetXMLName();
  if (group == "views" && vtkSMViewProxy::SafeDownCast(proxy))
  {
    return CreatePQView(group, name, vtkSMViewProxy::SafeDownCast(proxy), server);
  }
  else if (group == "layouts")
  {
    return new pqProxy(group, name, proxy, server, nullptr);
  }
  else if (group == "sources")
  {
    if (pqPipelineFilter::getInputPorts(proxy).size() > 0)
    {
      return new pqPipelineFilter(name, proxy, server, nullptr);
    }
    else
    {
      return new pqPipelineSource(name, proxy, server, nullptr);
    }
  }
  else if (group == "extractors" && xml_type == "Extractor")
  {
    return new pqExtractor(group, name, proxy, server);
  }
  else if (group == "timekeeper")
  {
    return new pqTimeKeeper(group, name, proxy, server, nullptr);
  }
  else if (group == "lookup_tables")
  {
    return new pqScalarsToColors(group, name, proxy, server, nullptr);
  }
  else if (group == "scalar_bars")
  {
    return new pqScalarBarRepresentation(group, name, proxy, server, nullptr);
  }
  else if (group == "representations")
  {
    if (proxy->IsA("vtkSMRepresentationProxy") && proxy->GetProperty("Input"))
    {
      if (proxy->IsA("vtkSMPVRepresentationProxy") || xml_type == "ImageSliceRepresentation")
      {
        // pqPipelineRepresentation is a design flaw! We need to get rid of it
        // and have helper code that manages the crap in that class
        return new pqPipelineRepresentation(group, name, proxy, server, nullptr);
      }

      // If everything fails, simply create a pqDataRepresentation object.
      return new pqDataRepresentation(group, name, proxy, server, nullptr);
    }
  }
  else if (group == "animation")
  {
    if (xml_type == "AnimationScene")
    {
      return new pqAnimationScene(group, name, proxy, server, nullptr);
    }
    else if (xml_type == "KeyFrameAnimationCue" || xml_type == "CameraAnimationCue" ||
      xml_type == "TimeAnimationCue" || xml_type == "PythonAnimationCue")
    {
      return new pqAnimationCue(group, name, proxy, server, nullptr);
    }
  }
  return nullptr;
}
