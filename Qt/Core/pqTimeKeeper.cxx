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
#include "pqSMAdaptor.h"
#include "pqView.h"

#include <vtkstd/vector>
//-----------------------------------------------------------------------------
class pqTimeKeeper::pqInternals
{
public:
  typedef QMap<double, QList<QPointer<pqPipelineSource> > > TimeMapType;
  typedef TimeMapType::iterator TimeMapIteratorType;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  TimeMapType Timesteps;
  TimeMapType Timeranges;

  static void clearValues(TimeMapType &map, pqPipelineSource *value)
  {
    TimeMapIteratorType iter = map.begin();
    while (iter != map.end())
      {
      if (iter.value().contains(value))
        {
        iter.value().removeAll(value);
        if (iter.value().size() == 0)
          {
          iter = map.erase(iter);
          continue;
          }
        }
      ++iter;
      }
  }
  static void insertValue(TimeMapType &map, double key, pqPipelineSource *value)
  {
    TimeMapIteratorType iter = map.find(key);
    if (iter != map.end())
      {
      iter.value().push_back(value);
      }
    else
      {
      QList<QPointer<pqPipelineSource> > valueList;
      valueList.push_back(value);
      map.insert(key, valueList);
      }
  }
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
    }

  emit this->timeChanged();
}

//-----------------------------------------------------------------------------
pqTimeKeeper::~pqTimeKeeper()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getNumberOfTimeStepValues() const
{
  return this->Internals->Timesteps.size();
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTimeStepValue(int index) const
{
  if (index < this->Internals->Timesteps.size())
    {
    QList<double> keys = this->Internals->Timesteps.keys();
    return keys[index];
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getTimeStepValueIndex(double time) const
{
  QList<double> keys = this->Internals->Timesteps.keys();
  int cc=1;
  for (cc=1; cc < keys.size(); cc++)
    {
    if (keys[cc] > time)
      {
      return (cc-1);
      }
    }
  return (cc-1); 
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::updateTimeKeeperProxy()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimestepValues"));


  QVector<double> keys = this->Internals->Timesteps.keys().toVector();
  vtkstd::vector<double> std_keys = keys.toStdVector();

  dvp->SetNumberOfElements(this->Internals->Timesteps.size());
  if (this->Internals->Timesteps.size() != 0)
    {
    dvp->SetElements(&std_keys[0]);
    }
  this->getProxy()->UpdateVTKObjects();

  // if the current time is not in the range of the timesteps currently
  // available, we change the time.
  QPair<double, double> range = this->getTimeRange();
  double curtime = this->getTime();
  if (range.first < range.second && 
    (curtime < range.first || curtime > range.second))
    {
    this->setTime(range.first);
    }

  emit this->timeStepsChanged();
}

//-----------------------------------------------------------------------------
QPair<double, double> pqTimeKeeper::getTimeRange() const
{
  if (this->Internals->Timeranges.size() == 0)
    {
    return QPair<double, double>(0.0, 0.0);
    }

  return QPair<double, double>(this->Internals->Timeranges.begin().key(),
    (this->Internals->Timeranges.end()-1).key());
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTime() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Time")).toDouble();  
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::setTime(double time) 
{
  pqSMAdaptor::setElementProperty(
    this->getProxy()->GetProperty("Time"), time);
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::sourceAdded(pqPipelineSource* source)
{
  vtkSMProxy* proxy = source->getProxy();
  if (proxy->GetProperty("TimestepValues") || proxy->GetProperty("TimeRange"))
    {
    this->Internals->VTKConnect->Connect(proxy, vtkCommand::PropertyModifiedEvent,
      this, SLOT(propertyModified(vtkObject*, unsigned long, void*, void*)));

    this->propertyModified(source);
    }

}
//-----------------------------------------------------------------------------
void pqTimeKeeper::propertyModified(vtkObject* obj, unsigned long, void*, void* callData)
{
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);
  const char* name = reinterpret_cast<const char*>(callData);
  if (   !proxy || !name
      || (   (strcmp(name, "TimestepValues") != 0)
          && (strcmp(name, "TimeRange") != 0) ) )
    {
    return;
    }

  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource* source = smmodel->findItem<pqPipelineSource*>(proxy);
  if (source)
    {
    this->propertyModified(source);
    }
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::propertyModified(pqPipelineSource* source)
{
  // The important thing to note here is that pqTimeKeeper never itself calls
  // UpdatePipelineInformation() or UpdatePropertyInformation(). It merely
  // updates the timekeeper whenever any reader reports it have timesteps
  // available or they are changed. This is crucial since this method may
  // get called during undo/redo, loading state etc when the proxy may not
  // have necessary properties intialized to do a UpdatePropertyInformation().

  vtkSMProxy* proxy = source->getProxy();
  this->cleanupTimes(source);

  if (proxy->GetProperty("TimestepValues"))
    {
    QList<QVariant> timestepValues = pqSMAdaptor::getMultipleElementProperty(
                                          proxy->GetProperty("TimestepValues"));
    if (timestepValues.size() > 0)
      {
      foreach (QVariant vtime, timestepValues)
        {
        pqInternals::insertValue(this->Internals->Timesteps,
                                 vtime.toDouble(), source);
        }
      // The following may result in multiple entries in the Timeranges map for
      // sources with both TimestepValues and TimeRanges properties, but that is
      // OK.
      pqInternals::insertValue(this->Internals->Timeranges,
                               timestepValues.first().toDouble(), source);
      pqInternals::insertValue(this->Internals->Timeranges,
                               timestepValues.last().toDouble(), source);
    }
    }

  if (proxy->GetProperty("TimeRange"))
    {
    QList<QVariant> timeRange = pqSMAdaptor::getMultipleElementProperty(
                                               proxy->GetProperty("TimeRange"));
    if (timeRange.size() == 2)
      {
      pqInternals::insertValue(this->Internals->Timeranges,
                               timeRange[0].toDouble(), source);
      pqInternals::insertValue(this->Internals->Timeranges,
                               timeRange[1].toDouble(), source);
      }
    }

  this->updateTimeKeeperProxy();
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::sourceRemoved(pqPipelineSource* source)
{
  this->Internals->VTKConnect->Disconnect(source->getProxy());
  this->cleanupTimes(source);
  this->updateTimeKeeperProxy();
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::cleanupTimes(pqPipelineSource* source)
{
  // Remove times reported by this source.
  pqInternals::clearValues(this->Internals->Timesteps, source);
  pqInternals::clearValues(this->Internals->Timeranges, source);
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::viewAdded(pqView* view)
{
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
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Views"));
  if (pp->IsProxyAdded(view->getProxy()))
    {
    pp->RemoveProxy(view->getProxy());
    this->getProxy()->UpdateProperty("Views");
    }
}
