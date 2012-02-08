/*=========================================================================

   Program: ParaView
   Module:    pqTimeKeeper.cxx

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
#include "pqTimeKeeper.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"

#include <QList>
#include <QMap>
#include <QPointer>
#include <QtDebug>
#include <QVector>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include <vector>
//-----------------------------------------------------------------------------
class pqTimeKeeper::pqInternals
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqTimeKeeper::pqTimeKeeper( const QString& group, const QString& name,
  vtkSMProxy* timekeeper, pqServer* server, QObject* _parent/*=0*/)
  : pqProxy(group, name, timekeeper, server, _parent)
{
  this->Internals = new pqInternals();
  this->Internals->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("Time"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimestepValues"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeStepsChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimestepValues"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeRangeChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimeRange"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeRangeChanged()));

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this, SLOT(sourceAdded(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this, SLOT(sourceRemoved(pqPipelineSource*)));

  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)),
    this, SLOT(viewAdded(pqView*)));
  QObject::connect(smmodel, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(viewRemoved(pqView*)));

  this->blockSignals(true);
  // ServerManagerModel may already have some registered sources
  // (happens when loading state).
  // So we pretend that every one of the sources is getting
  // newly added.
  QList<pqPipelineSource*> sources = smmodel->findItems<pqPipelineSource*>(
    this->getServer());
  foreach(pqPipelineSource* src, sources)
    {
    this->sourceAdded(src);
    }

  QList<pqView*> views = smmodel->findItems<pqView*>(this->getServer());
  foreach (pqView* view, views)
    {
    this->viewAdded(view);
    }
  this->blockSignals(false);

  if (sources.size() > 0)
    {
    emit this->timeStepsChanged();
    emit this->timeRangeChanged();
    }

  emit this->timeChanged();
}

//-----------------------------------------------------------------------------
pqTimeKeeper::~pqTimeKeeper()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
QList<double> pqTimeKeeper::getTimeSteps() const
{
  vtkSMPropertyHelper helper(this->getProxy(), "TimestepValues");
  QList<double> list;
  for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
    {
    list.push_back(helper.GetAsDouble(cc));
    }
  return list;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getNumberOfTimeStepValues() const
{
  return vtkSMPropertyHelper(this->getProxy(),
    "TimestepValues").GetNumberOfElements();
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTimeStepValue(int index) const
{
  if (index < this->getNumberOfTimeStepValues())
    {
    return vtkSMPropertyHelper(this->getProxy(),
      "TimestepValues").GetAsDouble(index);
    }

  return 0.0;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getTimeStepValueIndex(double time) const
{
  int num_values = this->getNumberOfTimeStepValues();
  double *values = new double[num_values+1];
  vtkSMPropertyHelper(this->getProxy(), "TimestepValues").Get(values,
    num_values);

  int cc=1;
  for (cc=1; cc < num_values; cc++)
    {
    if (values[cc] > time)
      {
      delete[] values;
      return (cc-1);
      }
    }
  delete[] values;
  return (cc-1); 
}

//-----------------------------------------------------------------------------
QPair<double, double> pqTimeKeeper::getTimeRange() const
{
  vtkSMPropertyHelper helper(this->getProxy(), "TimeRange");
  return QPair<double, double>(helper.GetAsDouble(0),
    helper.GetAsDouble(1));
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTime() const
{
  return vtkSMPropertyHelper(this->getProxy(), "Time").GetAsDouble(0);
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::setTime(double time) 
{
  vtkSMPropertyHelper(this->getProxy(), "Time").Set(time);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::sourceAdded(pqPipelineSource* source)
{
  if (!source || source->getServer() != this->getServer())
    {
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimeSources"));
  if (!pp->IsProxyAdded(source->getProxy()))
    {
    pp->AddProxy(source->getProxy());
    this->getProxy()->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::sourceRemoved(pqPipelineSource* source)
{
  if (!source || source->getServer() != this->getServer())
    {
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimeSources"));
  pp->RemoveProxy(source->getProxy());
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqTimeKeeper::isSourceAdded(pqPipelineSource* source)
{
  if (!source || source->getServer() != this->getServer())
    {
    return false;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimeSources"));
  return (source && pp->IsProxyAdded(source->getProxy()));
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::viewAdded(pqView* view)
{
  if (view->getServer() != this->getServer())
    {
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Views"));
  if (!pp->IsProxyAdded(view->getProxy()))
    {
    pp->AddProxy(view->getProxy());
    this->getProxy()->UpdateProperty("Views");
    }
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::viewRemoved(pqView* view)
{
  if (view->getServer() != this->getServer())
    {
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Views"));
  while (pp->IsProxyAdded(view->getProxy()))
    {
    pp->RemoveProxy(view->getProxy());
    this->getProxy()->UpdateProperty("Views");
    }
}
