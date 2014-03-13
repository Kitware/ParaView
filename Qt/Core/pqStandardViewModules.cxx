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

#include "pqBoxChartView.h"
#include "pqComparativeRenderView.h"
#include "pqComparativeXYBarChartView.h"
#include "pqComparativeXYChartView.h"
#include "pqMultiSliceView.h"
#include "pqParallelCoordinatesChartView.h"
#include "pqPlotMatrixView.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqSpreadSheetView.h"
#include "pqXYBagChartView.h"
#include "pqXYBarChartView.h"
#include "pqXYFunctionalBagChartView.h"
#include "pqXYChartView.h"
#include "vtkPVConfig.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMContextViewProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"

#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
#include "pqPythonView.h"
#endif

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
    pqSpreadSheetView::spreadsheetViewType() <<
    pqXYChartView::XYChartViewType() <<
    pqXYBagChartView::XYBagChartViewType() <<
    pqXYBarChartView::XYBarChartViewType() <<
    pqBoxChartView::chartViewType() <<
    pqComparativeRenderView::comparativeRenderViewType() <<
    pqComparativeXYChartView::chartViewType() <<
    pqComparativeXYBarChartView::chartViewType() <<
    pqXYFunctionalBagChartView::XYFunctionalBagChartViewType() <<
    pqParallelCoordinatesChartView::chartViewType() <<
    pqMultiSliceView::multiSliceViewType() <<
#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
    pqPythonView::pythonViewType() <<
#endif
    pqPlotMatrixView::viewType();
}

QString pqStandardViewModules::viewTypeName(const QString& type) const
{
  if (type == pqRenderView::renderViewType())
    {
    return pqRenderView::renderViewTypeName();
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
  else if (type == pqXYChartView::XYChartViewType())
    {
    return pqXYChartView::XYChartViewTypeName();
    }
  else if (type == pqXYBagChartView::XYBagChartViewType())
    {
    return pqXYBagChartView::XYBagChartViewTypeName();
    }
  else if (type == pqXYBarChartView::XYBarChartViewType())
    {
    return pqXYBarChartView::XYBarChartViewTypeName();
    }
  else if (type == pqBoxChartView::chartViewType())
    {
    return pqBoxChartView::chartViewTypeName();
    }
  else if (type == pqXYFunctionalBagChartView::XYFunctionalBagChartViewType())
    {
    return pqXYFunctionalBagChartView::XYFunctionalBagChartViewTypeName();
    }
  else if (type == pqParallelCoordinatesChartView::chartViewType())
    {
    return pqParallelCoordinatesChartView::chartViewTypeName();
    }
  else if (type == pqPlotMatrixView::viewType())
    {
    return pqPlotMatrixView::viewTypeName();
    }
  else if (type == pqMultiSliceView::multiSliceViewType())
    {
    return pqMultiSliceView::multiSliceViewTypeName();
    }
#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
  else if (type == pqPythonView::pythonViewType())
    {
    return pqPythonView::pythonViewTypeName();
    }
#endif

  return QString();
}

bool pqStandardViewModules::canCreateView(const QString& viewtype) const
{
  return this->viewTypes().contains(viewtype);
}

vtkSMProxy* pqStandardViewModules::createViewProxy(const QString& viewtype,
                                                   pqServer *server)
{
  vtkSMSessionProxyManager* pxm = server->proxyManager();
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
  else if (viewtype == pqSpreadSheetView::spreadsheetViewType())
    {
    root_xmlname = "SpreadSheetView";
    }
  else if (viewtype == pqXYChartView::XYChartViewType())
    {
    root_xmlname = "XYChartView";
    }
  else if (viewtype == pqXYBagChartView::XYBagChartViewType())
    {
    root_xmlname = "XYBagChartView";
    }
  else if (viewtype == pqXYBarChartView::XYBarChartViewType())
    {
    root_xmlname = "XYBarChartView";
    }
  else if (viewtype == pqBoxChartView::chartViewType())
    {
    root_xmlname = "BoxChartView";
    }
  else if (viewtype == pqXYFunctionalBagChartView::XYFunctionalBagChartViewType())
    {
    root_xmlname = "XYFunctionalBagChartView";
    }
  else if (viewtype == pqParallelCoordinatesChartView::chartViewType())
    {
    root_xmlname = "ParallelCoordinatesChartView";
    }
  else if (viewtype == pqPlotMatrixView::viewType())
    {
    root_xmlname = "PlotMatrixView";
    }
  else if (viewtype == pqMultiSliceView::multiSliceViewType())
    {
    root_xmlname = "MultiSlice";
    }
#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
  else if (viewtype == pqPythonView::pythonViewType())
    {
    root_xmlname = "PythonView";
    }
#endif

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
  else if (viewtype == pqMultiSliceView::multiSliceViewType())
    {
    return new pqMultiSliceView(viewtype,
                                group,
                                viewname,
                                viewmodule,
                                server,
                                p);
    }
#if defined(PARAVIEW_ENABLE_PYTHON) && defined(PARAVIEW_ENABLE_MATPLOTLIB)
  else if (viewtype == pqPythonView::pythonViewType())
    {
    return new pqPythonView(viewtype, group, viewname, viewmodule, server, p);
    }
#endif
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
  else if (viewtype == "XYChartView")
    {
    return new pqXYChartView(group, viewname,
                            vtkSMContextViewProxy::SafeDownCast(viewmodule),
                            server, p);
    }
  else if (viewtype == "XYBagChartView")
    {
    return new pqXYBagChartView(group, viewname,
                                vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                server, p);
    }
  else if (viewtype == "XYBarChartView")
    {
    return new pqXYBarChartView(group, viewname,
                                vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                server, p);
    }
  else if (viewtype == "BoxChartView")
    {
    return new pqBoxChartView(group, viewname,
                              vtkSMContextViewProxy::SafeDownCast(viewmodule),
                              server, p);
    }
  else if (viewtype == "XYFunctionalBagChartView")
    {
    return new pqXYFunctionalBagChartView(group, viewname,
                                vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                server, p);
    }
  else if (viewtype == "ParallelCoordinatesChartView")
    {
    return new pqParallelCoordinatesChartView(group, viewname,
                                        vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                        server, p);
    }
  else if (viewtype == "PlotMatrixView")
    {
    return new pqPlotMatrixView(group,
                                viewname,
                                vtkSMContextViewProxy::SafeDownCast(viewmodule),
                                server,
                                p);
    }

  qDebug() << "Failed to create a proxy" << viewmodule->GetClassName();
  return NULL;
}
