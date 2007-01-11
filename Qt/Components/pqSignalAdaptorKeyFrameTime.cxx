/*=========================================================================

   Program:   ParaView
   Module:    pqSignalAdaptorKeyFrameTime.cxx

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
#include "pqSignalAdaptorKeyFrameTime.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"

#include <QPointer>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqSMAdaptor.h"

class pqSignalAdaptorKeyFrameTime::pqInternals
{
public:
  QPointer<pqAnimationScene> Scene;
  QPointer<pqAnimationCue> Cue;
  QString PropertyName;
  vtkEventQtSlotConnect* VTKConnect;
  double LastTime;
  pqInternals()
    {
    this->VTKConnect = vtkEventQtSlotConnect::New();
    this->LastTime = 0;
    }
  ~pqInternals()
    {
    this->VTKConnect->Delete();
    }
};

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameTime::pqSignalAdaptorKeyFrameTime(QObject* object,
  const QString& propertyname, const QString& signal)
: QObject(object)
{
  this->Internals = new pqInternals();
  this->Internals->PropertyName = propertyname;

  QObject::connect(object, signal.toAscii().data(), 
    this, SIGNAL(timeChanged()));
}

//-----------------------------------------------------------------------------
pqSignalAdaptorKeyFrameTime::~pqSignalAdaptorKeyFrameTime()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameTime::setNormalizedTime(double ntime)
{
  double scaled_time = ntime;
  if (this->Internals->Scene && this->Internals->Cue)
    {
    vtkSMProxy* scene = this->Internals->Scene->getProxy();
    vtkSMProxy* cue = this->Internals->Cue->getProxy();

    if (pqSMAdaptor::getEnumerationProperty(
        cue->GetProperty("TimeMode")) == "Normalized")
      {
      double start = pqSMAdaptor::getElementProperty(
        scene->GetProperty("StartTime")).toDouble();
      double end = pqSMAdaptor::getElementProperty(
        scene->GetProperty("EndTime")).toDouble();
      scaled_time = start + ntime*(end-start);
      }
    }
  if (this->parent()->property(
      this->Internals->PropertyName.toAscii().data()).toDouble() != scaled_time)
    {
    this->parent()->setProperty(
      this->Internals->PropertyName.toAscii().data(), scaled_time);
    }
  this->Internals->LastTime = ntime;
}

//-----------------------------------------------------------------------------
double pqSignalAdaptorKeyFrameTime::normalizedTime() const
{
  double time = this->parent()->property(
      this->Internals->PropertyName.toAscii().data()).toDouble();
  if (this->Internals->Scene  && this->Internals->Cue)
    {
    vtkSMProxy* scene = this->Internals->Scene->getProxy();
    vtkSMProxy* cue = this->Internals->Cue->getProxy();

    if (pqSMAdaptor::getEnumerationProperty(
        cue->GetProperty("TimeMode")) == "Normalized")
      {
      double start = pqSMAdaptor::getElementProperty(
        scene->GetProperty("StartTime")).toDouble();
      double end = pqSMAdaptor::getElementProperty(
        scene->GetProperty("EndTime")).toDouble();
      if (start != end)
        {
        time = (time - start)/(end-start);
        }
      }
    }
  return time;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameTime::setAnimationCue(pqAnimationCue* cue)
{
  this->Internals->Cue = cue;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameTime::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Internals->Scene)
    {
    QObject::disconnect(this->Internals->Scene, 0, this, 0);
    }
  this->Internals->Scene = scene;
  if (this->Internals->Scene)
    {
    QObject::connect(scene, SIGNAL(startTimeChanged()),
      this, SLOT(sceneChanged()));
    QObject::connect(scene, SIGNAL(endTimeChanged()),
      this, SLOT(sceneChanged()));
    }
}
//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameTime::sceneChanged()
{
  this->setNormalizedTime(this->Internals->LastTime);
}

