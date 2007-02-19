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

#include "pqPlotViewModule.h"
#include "pqTableViewModule.h"
#include "pqBarChartDisplay.h"

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
    pqPlotViewModule::barChartType() << 
    pqPlotViewModule::XYPlotType() << 
    pqTableViewModule::tableType();
}

QStringList pqStandardViewModules::displayTypes() const
{
  return QStringList() << "BarChartDisplay";
}

QString pqStandardViewModules::viewTypeName(const QString& type) const
{
  if(type == pqPlotViewModule::barChartType())
    return pqPlotViewModule::barChartTypeName();
  else if(type == pqPlotViewModule::XYPlotType())
    return pqPlotViewModule::XYPlotTypeName();
  else if(type == pqTableViewModule::tableType())
    return pqTableViewModule::tableTypeName();

  return QString();
}

bool pqStandardViewModules::canCreateView(const QString& viewtype) const
{
  return this->viewTypes().contains(viewtype);
}

vtkSMProxy* pqStandardViewModules::createViewProxy(const QString& viewtype)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if(viewtype == pqPlotViewModule::barChartType())
    {
    return pxm->NewProxy("plotmodules", "BarChartViewModule");
    }
  else if(viewtype == pqPlotViewModule::XYPlotType())
    {
    return pxm->NewProxy("plotmodules", "XYPlotViewModule");
    }
  else if(viewtype == pqTableViewModule::tableType())
    {
    return pxm->NewProxy("views", "TableView");
    }

  return NULL;
}


pqGenericViewModule* pqStandardViewModules::createView(const QString& viewtype,
                                                const QString& group,
                                                const QString& viewname,
                                                vtkSMAbstractViewModuleProxy* viewmodule,
                                                pqServer* server,
                                                QObject* p)
{
  if(viewtype == "BarChart")
    {
    return new pqPlotViewModule(pqPlotViewModule::barChartType(),
                              group, viewname, viewmodule, server, p);
    }
  else if(viewtype == "XYPlot")
    {
    return new pqPlotViewModule(pqPlotViewModule::XYPlotType(),
                              group, viewname, viewmodule, server, p);
    }
  else if(viewtype == "TableView")
    {
    return new pqTableViewModule(group, viewname, viewmodule, server, p);
    }

  return NULL;
}

pqConsumerDisplay* pqStandardViewModules::createDisplay(const QString& display_type, 
  const QString& group,
  const QString& n,
  vtkSMProxy* proxy,
  pqServer* server,
  QObject* p)
{
  if(display_type == "BarChartDisplay")
    {
    return new pqBarChartDisplay(group, n, proxy, server, p);
    }

  return NULL;
}


