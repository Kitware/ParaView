/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewModules.cxx

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

=========================================================================*/

#include "pqStandardViewModules.h"

#include "pqChartRepresentation.h"
#include "pqComparativeRenderView.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqSpreadSheetView.h"
#include "pqTableView.h"
#include "pqTextRepresentation.h"
#include "pqTwoDRenderView.h"
//#include "pqScatterPlotView.h"
#include "pqXYChartView.h"
#include "pqXYBarChartView.h"
#include "pqComparativeXYChartView.h"
#include "pqComparativeXYBarChartView.h"
#include "pqParallelCoordinatesChartView.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"

#include <QDebug>

pqStandardViewModules::pqStandardViewModules(QObject* o)
  : QObject(o)
{
}

pqStandardViewModules::~pqStandardViewModules()
{
}

QStringList pqStandardViewModules::viewTypes() const
{
  return QStringList() <<
    pqRenderView::renderViewType() <<
    pqTwoDRenderView::twoDRenderViewType() <<
    pqTableView::tableType() <<
    pqSpreadSheetView::spreadsheetViewType() <<
//    pqScatterPlotView::scatterPlotViewType() <<
    pqXYChartView::XYChartViewType() <<
    pqXYBarChartView::XYBarChartViewType() <<
    pqComparativeRenderView::comparativeRenderViewType() <<
    pqComparativeXYChartView::chartViewType() <<
    pqComparativeXYBarChartView::chartViewType() <<
    pqParallelCoordinatesChartView::chartViewType();
}

QStringList pqStandardViewModules::displayTypes() const
{
  return QStringList()
    << "XYChartRepresentation"
    << "XYBarChartRepresentation"
    << "TextSourceRepresentation";
}

QString pqStandardViewModules::viewTypeName(const QString& type) const
{
  if (type == pqRenderView::renderViewType())
    {
    return pqRenderView::renderViewTypeName();
    }
  else if(type == pqTableView::tableType())
    {
    return pqTableView::tableTypeName();
    }
  else if (type == pqComparativeRenderView::comparativeRenderViewType())
    {
    return pqComparativeRenderView::comparativeRenderViewTypeName();
    }
  else if (type == pqComparativeXYBarChartView::chartViewType())
    {
    return pqComparativeXYBarChartView::chartViewTypeName();
    }
  else if (type == pqComparativeXYChartView::chartViewType())
    {
    return pqComparativeXYChartView::chartViewTypeName();
    }
  else if (type == pqSpreadSheetView::spreadsheetViewType())
    {
    return pqSpreadSheetView::spreadsheetViewTypeName();
    }
  else if  (type == pqTwoDRenderView::twoDRenderViewType())
    {
    return pqTwoDRenderView::twoDRenderViewTypeName();
    }
//  else if  (type == pqScatterPlotView::scatterPlotViewType())
//    {
//    return pqScatterPlotView::scatterPlotViewTypeName();
//    }
  else if (type == pqXYChartView::XYChartViewType())
    {
    return pqXYChartView::XYChartViewTypeName();
    }
  else if (type == pqXYBarChartView::XYBarChartViewType())
    {
    return pqXYBarChartView::XYBarChartViewTypeName();
    }
  else if (type == pqParallelCoordinatesChartView::chartViewType())
    {
    return pqParallelCoordinatesChartView::chartViewTypeName();
    }

  return QString();
}

bool pqStandardViewModules::canCreateView(const QString& viewtype) const
{
  return this->viewTypes().contains(viewtype);
}

vtkSMProxy* pqStandardViewModules::createViewProxy(const QString& viewtype,
                                                   pqServer *server)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  const char* root_xmlname = 0;
  if(viewtype == pqRenderView::renderViewType())
    {
    root_xmlname = "RenderView";
    }
  else if(viewtype == pqComparativeRenderView::comparativeRenderViewType())
    {
    root_xmlname = "ComparativeRenderView";
    }
  else if(viewtype == pqComparativeXYBarChartView::chartViewType())
    {
    root_xmlname = "ComparativeXYBarChartView";
    }
  else if(viewtype == pqComparativeXYChartView::chartViewType())
    {
    root_xmlname = "ComparativeXYChartView";
    }
  else if (viewtype == pqTwoDRenderView::twoDRenderViewType())
    {
    root_xmlname = "2DRenderView";
    }
  else if(viewtype == pqTableView::tableType())
    {
    root_xmlname = "TableView";
    }
  else if (viewtype == pqSpreadSheetView::spreadsheetViewType())
    {
    root_xmlname = "SpreadSheetView";
    }
//  else if (viewtype == pqScatterPlotView::scatterPlotViewType())
//    {
//    root_xmlname = "ScatterPlotRenderView";
//    }
  else if (viewtype == pqXYChartView::XYChartViewType())
    {
    root_xmlname = "XYChartView";
    }
  else if (viewtype == pqXYBarChartView::XYBarChartViewType())
    {
    root_xmlname = "XYBarChartView";
    }
  else if (viewtype == pqParallelCoordinatesChartView::chartViewType())
    {
    root_xmlname = "ParallelCoordinatesChartView";
    }

  if (root_xmlname)
    {
    return pxm->NewProxy("views", root_xmlname);
    }

  return NULL;
}


pqView* pqStandardViewModules::createView(const QString& viewtype,
                                          const QString& group,
                                          const QString& viewname,
                                          vtkSMViewProxy* viewmodule,
                                          pqServer* server,
                                          QObject* p)
{
  if(viewtype == "TableView")
    {
    // return new pqTableView(group, viewname, viewmodule, server, p);
    }
  else if (viewtype == pqSpreadSheetView::spreadsheetViewType())
    {
    return new pqSpreadSheetView(
      group, viewname, viewmodule, server, p);
    }
  else if (viewmodule->IsA("vtkSMTwoDRenderViewProxy"))
    {
    return new pqTwoDRenderView(
      group, viewname, viewmodule, server, p);
    }
  else if (viewmodule->IsA("vtkSMRenderViewProxy"))
    {
    return new pqRenderView(group, viewname, viewmodule, server, p);
    }
  else if (viewtype == pqComparativeXYBarChartView::chartViewType()
    && viewmodule->IsA("vtkSMComparativeViewProxy"))
    {
    return new pqComparativeXYBarChartView(
      group, viewname, vtkSMComparativeViewProxy::SafeDownCast(viewmodule),
      server, p);
    }
  else if (viewtype == pqComparativeXYChartView::chartViewType()
    && viewmodule->IsA("vtkSMComparativeViewProxy"))
    {
    return new pqComparativeXYChartView(
      group, viewname, vtkSMComparativeViewProxy::SafeDownCast(viewmodule),
      server, p);
    }
  else if (viewmodule->IsA("vtkSMComparativeViewProxy"))
    {
    // Handle the other comparative render views.
    return new pqComparativeRenderView(
      group, viewname, viewmodule, server, p);
    }
//  else if (viewmodule->IsA("vtkSMScatterPlotViewProxy"))
//    {
//    return new pqScatterPlotView(
//      group, viewname, viewmodule, server, p);
//    }
  else if (viewtype == "XYChartView")
    {
    return new pqXYChartView(group, viewname,
                            vtkSMContextViewProxy::SafeDownCast(viewmodule),
                            server, p);
    }
  else if (viewtype == "XYBarChartView")
    {
    return new pqXYBarChartView(group, viewname,
                                vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                server, p);
    }
  else if (viewtype == "ParallelCoordinatesChartView")
    {
    return new pqParallelCoordinatesChartView(group, viewname,
                                        vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                        server, p);
    }

  qDebug() << "Failed to create a proxy" << viewmodule->GetClassName();
  return NULL;
}

pqDataRepresentation* pqStandardViewModules::createDisplay(const QString& display_type,
  const QString& group,
  const QString& n,
  vtkSMProxy* proxy,
  pqServer* server,
  QObject* p)
{
  if (display_type == "XYChartRepresentation" ||
      display_type == "XYBarChartRepresentation")
    {
    // new chart representations.
    return new pqChartRepresentation(group, n, proxy, server, p);
    }
  else if (display_type == "TextSourceRepresentation")
    {
    return new pqTextRepresentation(group, n, proxy, server, p);
    }

  return NULL;
}
