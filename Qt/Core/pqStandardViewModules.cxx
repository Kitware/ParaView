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

#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"

#include "pqBarChartRepresentation.h"
#include "pqComparativeRenderView.h"
#include "pqComparativePlotView.h"
#include "pqLineChartRepresentation.h"
#include "pqPlotView.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqSpreadSheetView.h"
#include "pqTableView.h"
#include "pqTextRepresentation.h"
#include "pqTwoDRenderView.h"

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
    pqPlotView::barChartType() << 
    pqPlotView::XYPlotType() << 
    pqTableView::tableType() <<
    pqComparativeRenderView::comparativeRenderViewType() <<
    pqComparativePlotView::comparativeXYPlotViewType() <<
    pqComparativePlotView::comparativeBarChartViewType() <<
    pqSpreadSheetView::spreadsheetViewType();
}

QStringList pqStandardViewModules::displayTypes() const
{
  return QStringList() 
    << "BarChartRepresentation"
    << "XYPlotRepresentation"
    << "TextSourceRepresentation";
}

QString pqStandardViewModules::viewTypeName(const QString& type) const
{
  if (type == pqRenderView::renderViewType())
    {
    return pqRenderView::renderViewTypeName();
    }
  else if(type == pqPlotView::barChartType())
    {
    return pqPlotView::barChartTypeName();
    }
  else if(type == pqPlotView::XYPlotType())
    {
    return pqPlotView::XYPlotTypeName();
    }
  else if(type == pqTableView::tableType())
    {
    return pqTableView::tableTypeName();
    }
  else if (type == pqComparativeRenderView::comparativeRenderViewType())
    {
    return pqComparativeRenderView::comparativeRenderViewTypeName();
    }
  else if (type == pqComparativePlotView::comparativeXYPlotViewType())
    {
    return pqComparativePlotView::comparativeXYPlotViewTypeName();
    }
  else if (type == pqComparativePlotView::comparativeBarChartViewType())
    {
    return pqComparativePlotView::comparativeBarChartViewTypeName();
    }
  else if (type == pqSpreadSheetView::spreadsheetViewType())
    {
    return pqSpreadSheetView::spreadsheetViewTypeName();
    }
  else if  (type == pqTwoDRenderView::twoDRenderViewType())
    {
    return pqTwoDRenderView::twoDRenderViewTypeName();
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
  else if(viewtype == pqComparativePlotView::comparativeXYPlotViewType())
    {
    root_xmlname = "ComparativeXYPlotView";
    }
  else if(viewtype == pqComparativePlotView::comparativeBarChartViewType())
    {
    root_xmlname = "ComparativeBarChartView";
    }
  else if (viewtype == pqTwoDRenderView::twoDRenderViewType())
    {
    root_xmlname = "2DRenderView";
    }
  else if(viewtype == pqPlotView::barChartType())
    {
    root_xmlname = "BarChartView";
    }
  else if(viewtype == pqPlotView::XYPlotType())
    {
    root_xmlname = "XYPlotView";
    }
  else if(viewtype == pqTableView::tableType())
    {
    root_xmlname = "TableView";
    }
  else if (viewtype == pqSpreadSheetView::spreadsheetViewType())
    {
    root_xmlname = "SpreadSheetView";
    }

  if (root_xmlname)
    {
    vtkSMViewProxy* prototype = vtkSMViewProxy::SafeDownCast(
      pxm->GetPrototypeProxy("views", root_xmlname));
    if (prototype)
      {
      return pxm->NewProxy("views",
        prototype->GetSuggestedViewType(server->GetConnectionID()));
      }
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
  if(viewtype == pqPlotView::barChartType())
    {
    return new pqPlotView(pqPlotView::barChartType(),
                              group, viewname, viewmodule, server, p);
    }
  else if(viewtype == pqPlotView::XYPlotType())
    {
    return new pqPlotView(pqPlotView::XYPlotType(),
                              group, viewname, viewmodule, server, p);
    }
  else if(viewtype == "TableView")
    {
    // return new pqTableView(group, viewname, viewmodule, server, p);
    }

  else if (viewtype == pqSpreadSheetView::spreadsheetViewType())
    {
    return new pqSpreadSheetView(
      group, viewname, viewmodule, server, p);
    }
  else if (viewmodule->IsA("vtkSMRenderViewProxy"))
    {
    return new pqRenderView(group, viewname, viewmodule, server, p);
    }
  else if (viewtype == pqComparativePlotView::comparativeBarChartViewType())
    {
    return new pqComparativePlotView(pqPlotView::barChartType(),
                 group, viewname, viewmodule, server, p);
    }
  else if (viewtype == pqComparativePlotView::comparativeXYPlotViewType())
    {
    return new pqComparativePlotView(pqPlotView::XYPlotType(),
                 group, viewname, viewmodule, server, p);
    }
  else if (viewmodule->IsA("vtkSMComparativeViewProxy"))
    {
    // Handle the other comparative render views.
    return new pqComparativeRenderView(
      group, viewname, viewmodule, server, p);
    }
  else if (viewmodule->IsA("vtkSMTwoDRenderViewProxy"))
    {
    return new pqTwoDRenderView(
      group, viewname, viewmodule, server, p); 
    }

  return NULL;
}

pqDataRepresentation* pqStandardViewModules::createDisplay(const QString& display_type, 
  const QString& group,
  const QString& n,
  vtkSMProxy* proxy,
  pqServer* server,
  QObject* p)
{
  if(display_type == "BarChartRepresentation")
    {
    return new pqBarChartRepresentation(group, n, proxy, server, p);
    }
  else if (display_type == "XYPlotRepresentation")
    {
    return new pqLineChartRepresentation(group, n, proxy, server, p);
    }
  else if (display_type == "TextSourceRepresentation")
    {
    return new pqTextRepresentation(group, n, proxy, server, p);
    }

  return NULL;
}


