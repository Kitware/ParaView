/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewModule.cxx

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
#include "pqPlotViewModule.h"

#include <QtDebug>
#include <QPointer>

#include "pqHistogramWidget.h"

//-----------------------------------------------------------------------------
class pqPlotViewModuleInternal
{
public:
  QPointer<QWidget> PlotWidget; 
  pqPlotViewModuleInternal()
    {
    }
  ~pqPlotViewModuleInternal()
    {
    delete this->PlotWidget;
    }
};

//-----------------------------------------------------------------------------
pqPlotViewModule::pqPlotViewModule(int type,
  const QString& group, const QString& name, 
  vtkSMAbstractViewModuleProxy* renModule, pqServer* server, QObject* _parent)
: pqGenericViewModule(group, name, renModule, server, _parent)
{
  this->Type = type;
  this->Internal = new pqPlotViewModuleInternal();

  switch (this->Type)
    {
  case BAR_CHART:
    this->Internal->PlotWidget = new pqHistogramWidget();
    break;

  default:
    qDebug() << "PlotType: " << type << " not supported yet.";
    }
}

//-----------------------------------------------------------------------------
pqPlotViewModule::~pqPlotViewModule()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqPlotViewModule::getWidget() const
{
  return this->Internal->PlotWidget;
}

//-----------------------------------------------------------------------------
void pqPlotViewModule::setWindowParent(QWidget* p)
{
  if (this->Internal->PlotWidget)
    {
    this->Internal->PlotWidget->setParent(p);
    }
  else
    {
    qDebug() << "setWindowParent() failed since PlotWidget not yet created.";
    }
}
//-----------------------------------------------------------------------------
QWidget* pqPlotViewModule::getWindowParent() const
{
  if (this->Internal->PlotWidget)
    {
    return this->Internal->PlotWidget->parentWidget();
    }
  qDebug() << "getWindowParent() failed since PlotWidget not yet created.";
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
