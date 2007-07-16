/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewModules.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqBarChartRepresentation.h"
#include "pqComparativeRenderView.h"
#include "pqElementInspectorView.h"
#include "pqLineChartRepresentation.h"
#include "pqPlotView.h"
#include "pqTableView.h"
#include "pqTextRepresentation.h"

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
    pqPlotView::barChartType() << 
    pqPlotView::XYPlotType() << 
    pqTableView::tableType() <<
    pqElementInspectorView::eiViewType() <<
    pqComparativeRenderView::comparativeRenderViewType();
}

QStringList pqStandardViewModules::displayTypes() const
{
  return QStringList() 
    << "BarChartRepresentation"
    << "XYPlotRepresentation"
    << "ElementInspectorRepresentation"
    << "TextSourceRepresentation";
}

QString pqStandardViewModules::viewTypeName(const QString& type) const
{
  if(type == pqPlotView::barChartType())
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
  else if (type == pqElementInspectorView::eiViewType())
    {
    return pqElementInspectorView::eiViewTypeName();
    }
  else if (type == pqComparativeRenderView::comparativeRenderViewType())
    {
    return pqComparativeRenderView::comparativeRenderViewTypeName();
    }

  return QString();
}

bool pqStandardViewModules::canCreateView(const QString& viewtype) const
{
  return this->viewTypes().contains(viewtype);
}

vtkSMProxy* pqStandardViewModules::createViewProxy(const QString& viewtype)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if(viewtype == pqPlotView::barChartType())
    {
    return pxm->NewProxy("newviews", "BarChartView");
    }
  else if(viewtype == pqPlotView::XYPlotType())
    {
    return pxm->NewProxy("newviews", "XYPlotView");
    }
  else if(viewtype == pqTableView::tableType())
    {
    return pxm->NewProxy("views", "TableView");
    }
  else if (viewtype == pqElementInspectorView::eiViewType())
    {
    return pxm->NewProxy("newviews", "ElementInspectorView");
    }
  else if (viewtype == pqComparativeRenderView::comparativeRenderViewType())
    {
    return pxm->NewProxy("newviews", "ComparativeRenderView");
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
  else if (viewtype == "ElementInspectorView")
    {
    return new pqElementInspectorView(
      group, viewname, viewmodule, server, p);
    }
  else if (viewtype == pqComparativeRenderView::comparativeRenderViewType())
    {
    return new pqComparativeRenderView(
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
  else if (display_type == "ElementInspectorRepresentation")
    {
    return new pqDataRepresentation(group, n, proxy, server, p);
    }
  else if (display_type == "TextSourceRepresentation")
    {
    return new pqTextRepresentation(group, n, proxy, server, p);
    }

  return NULL;
}


