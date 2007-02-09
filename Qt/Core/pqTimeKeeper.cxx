/*=========================================================================

   Program: ParaView
   Module:    pqTimeKeeper.cxx

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
#include "pqTimeKeeper.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <QtDebug>
#include <QPointer>
#include <QMap>
#include <QList>

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqTimeKeeper::pqInternals
{
public:
  typedef QMap<double, QList<QPointer<pqPipelineSource> > > TimeMapType;
  typedef TimeMapType::iterator TimeMapIteratorType;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  TimeMapType Times;
};

//-----------------------------------------------------------------------------
pqTimeKeeper::pqTimeKeeper( const QString& group, const QString& name,
  vtkSMProxy* timekeeper, pqServer* server, QObject* _parent/*=0*/)
  : pqProxy(group, name, timekeeper, server, _parent)
{
  this->Internals = new pqInternals();
  this->Internals->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();


  QObject::connect(this, SIGNAL(triggerInitialization()),
    this, SLOT(delayedInitialization()), Qt::QueuedConnection);

  emit this->triggerInitialization();
}

//-----------------------------------------------------------------------------
pqTimeKeeper::~pqTimeKeeper()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::delayedInitialization()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this, SLOT(sourceAdded(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(sourceRemoved(pqPipelineSource*)),
    this, SLOT(sourceRemoved(pqPipelineSource*)));


  this->blockSignals(true);
  // ServerManagerModel may already have some registered sources
  // (happens when loading state).
  // So we pretend that every one of the sources is getting
  // newly added.
  QList<pqPipelineSource*> sources = smmodel->getSources(this->getServer());
  foreach(pqPipelineSource* src, sources)
    {
    this->sourceAdded(src);
    }
  this->blockSignals(false);
  emit this->timeStepsChanged();
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getNumberOfTimeStepValues() const
{
  return this->Internals->Times.size();
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::updateTimeKeeperProxy()
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->getProxy()->GetProperty("TimestepValues"));

  dvp->SetNumberOfElements(this->Internals->Times.size());
  pqInternals::TimeMapIteratorType iter = this->Internals->Times.begin();
  for (int cc=0; iter != this->Internals->Times.end(); ++iter, ++cc)
    {
    dvp->SetElement(cc, iter.key());
    }
  this->getProxy()->UpdateVTKObjects();
  emit this->timeStepsChanged();
}

//-----------------------------------------------------------------------------
QPair<double, double> pqTimeKeeper::getTimeRange() const
{
  if (this->Internals->Times.size() == 0)
    {
    return QPair<double, double>(0.0, 0.0);
    }

  return QPair<double, double>(this->Internals->Times.begin().key(),
    (this->Internals->Times.end()-1).key());
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTime() const
{
  return pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Time")).toDouble();  
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::sourceAdded(pqPipelineSource* source)
{
  vtkSMProxy* proxy = source->getProxy();
  if (proxy->GetProperty("TimestepValues"))
    {
    this->Internals->VTKConnect->Connect(proxy, vtkCommand::PropertyModifiedEvent,
      this, SLOT(propertyModified(vtkObject*, unsigned long, void*, void*)),
      0, 0, Qt::QueuedConnection);
    this->propertyModified(source);
    }

}
//-----------------------------------------------------------------------------
void pqTimeKeeper::propertyModified(vtkObject* obj, unsigned long, void*, void* callData)
{
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);
  const char* name = reinterpret_cast<const char*>(callData);
  if (!proxy || !name || strcmp(name, "TimestepValues") != 0)
    {
    return;
    }

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource* source = smmodel->getPQSource(proxy);
  this->propertyModified(source);
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::propertyModified(pqPipelineSource* source)
{
  vtkSMProxy* proxy = source->getProxy();
  this->cleanupTimes(source);
  QList<QVariant> timestepValues = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("TimestepValues"));
  foreach (QVariant vtime, timestepValues)
    {
    double time = vtime.toDouble();
    pqInternals::TimeMapIteratorType iter = this->Internals->Times.find(time);
    if (iter != this->Internals->Times.end())
      {
      iter.value().push_back(source);
      }
    else
      {
      QList<QPointer<pqPipelineSource> > value;
      value.push_back(source);
      this->Internals->Times.insert(time, value);
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
  pqInternals::TimeMapIteratorType iter = this->Internals->Times.begin();
  while (iter != this->Internals->Times.end())
    {
    if (iter.value().contains(source))
      {
      iter.value().removeAll(source);
      if (iter.value().size() == 0)
        {
        iter = this->Internals->Times.erase(iter);
        continue;
        }
      }
    ++iter;
    }
}
