/*=========================================================================

   Program: ParaView
   Module:    pqTableViewModule.cxx

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
#include "pqTableViewModule.h"

#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMProxy.h"

#include <QtDebug>
#include <QPointer>
#include <QTableView>

#include "pqDisplay.h"
#include "pqPipelineSource.h"

//-----------------------------------------------------------------------------
class pqTableViewModule::pqImplementation
{
public:
  pqImplementation() :
    Table(new QTableView())
  {
  }

  QPointer<QTableView> Table;
};

//-----------------------------------------------------------------------------
pqTableViewModule::pqTableViewModule(
    const QString& group,
    const QString& name, 
    vtkSMAbstractViewModuleProxy* renModule,
    pqServer* server,
    QObject* _parent) :
  pqGenericViewModule(group, name, renModule, server, _parent),
  Implementation(new pqImplementation())
{
/*
  switch (this->Type)
    {
  case BAR_CHART:
      {
      pqHistogramWidget* widget = new pqHistogramWidget();
      pqVTKHistogramModel* model = new pqVTKHistogramModel(this);
      widget->getHistogram().setModel(model);
      this->Internal->PlotWidget = widget;
      this->Internal->VTKModel = model;
      this->Internal->MaxNumberOfVisibleDisplays = 1;
      }
    break;

  case XY_PLOT:
      {
      pqLineChartWidget* widget = new pqLineChartWidget();
      pqVTKLineChartModel* model = new pqVTKLineChartModel(this);
      widget->getLineChart().setModel(model);
      this->Internal->PlotWidget = widget; 
      this->Internal->VTKModel = model;
      this->Internal->MaxNumberOfVisibleDisplays = -1;
      }
    break;

  default:
    qDebug() << "PlotType: " << type << " not supported yet.";
    }
  QObject::connect(this, SIGNAL(displayVisibilityChanged(pqDisplay*, bool)),
    this, SLOT(visibilityChanged(pqDisplay*)));
  QObject::connect(this, SIGNAL(displayAdded(pqDisplay*)),
    this, SLOT(visibilityChanged(pqDisplay*)));
*/
}

//-----------------------------------------------------------------------------
pqTableViewModule::~pqTableViewModule()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
QWidget* pqTableViewModule::getWidget()
{
  return this->Implementation->Table;
}

//-----------------------------------------------------------------------------
void pqTableViewModule::setWindowParent(QWidget* p)
{
/*
  if (this->Internal->PlotWidget)
    {
    this->Internal->PlotWidget->setParent(p);
    }
  else
    {
    qDebug() << "setWindowParent() failed since PlotWidget not yet created.";
    }
*/
}
//-----------------------------------------------------------------------------
QWidget* pqTableViewModule::getWindowParent() const
{
  return 0;
/*
  if (this->Internal->PlotWidget)
    {
    return this->Internal->PlotWidget->parentWidget();
    }
  qDebug() << "getWindowParent() failed since PlotWidget not yet created.";
  return 0;
*/
}

//-----------------------------------------------------------------------------
bool pqTableViewModule::canDisplaySource(pqPipelineSource* source) const
{
/*
  if (!this->Superclass::canDisplaySource(source))
    {
    return false;
    }
  switch (this->Type)
    {
  case BAR_CHART:
    return (source->getProxy()->GetXMLName() == QString("ExtractHistogram"));

  case XY_PLOT:
    return (source->getProxy()->GetXMLName() == QString("Probe2"));
    }
*/

  return false;
}

//-----------------------------------------------------------------------------
void pqTableViewModule::visibilityChanged(pqDisplay* disp)
{
/*
  if (disp->isVisible())
    {
    int max_visible = this->Internal->MaxNumberOfVisibleDisplays-1;
    int cc=0;
    QList<pqDisplay*> dislays = this->getDisplays();
    foreach(pqDisplay* d, dislays)
      {
      if (d != disp && d->isVisible())
        {
        cc++;
        if (cc > max_visible)
          {
          d->setVisible(false);
          }
        }
      }
    }
*/
}

//-----------------------------------------------------------------------------
void pqTableViewModule::forceRender()
{
  this->Superclass::forceRender();

/*
  // Now update the GUI.
  switch (this->Type)
    {
  case BAR_CHART:
    this->renderBarChar();
    break;

  case XY_PLOT:
    this->renderXYPlot();
    break;
    }
*/
}

/*
//-----------------------------------------------------------------------------
void pqTableViewModule::renderXYPlot()
{
  pqVTKLineChartModel* model = 
    qobject_cast<pqVTKLineChartModel*>(this->Internal->VTKModel);
  if (!model)
    {
    qDebug() << "Cannot locate pqVTKLineChartModel.";
    return;
    }

  QList<pqDisplay*> displays = this->getDisplays();
  QList<pqDisplay*> visibleDisplays;
  foreach (pqDisplay* display, displays)
    {
    if (display->isVisible())
      {
      visibleDisplays.push_back(display);
      }
    }
  model->update(visibleDisplays);
}

//-----------------------------------------------------------------------------
void pqTableViewModule::renderBarChar()
{
  pqVTKHistogramModel* model = 
    qobject_cast<pqVTKHistogramModel*>(this->Internal->VTKModel);
  if (!model)
    {
    qDebug() << "Cannot locate pqVTKHistogramModel.";
    return;
    }

  QList<pqDisplay*> displays = this->getDisplays();
  foreach (pqDisplay* display, displays)
    {
    if (display->isVisible())
      {
      vtkSMGenericViewDisplayProxy* disp = 
        vtkSMGenericViewDisplayProxy::SafeDownCast(display->getProxy());
      model->updateData(disp->GetOutput());
      return;
      }
    }
  model->updateData((vtkDataObject*)0);
}

*/
