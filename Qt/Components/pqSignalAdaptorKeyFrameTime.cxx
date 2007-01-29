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
  QPointer<pqAnimationScene> AnimationScene;
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
  if (this->Internals->AnimationScene && this->Internals->Cue)
    {
    vtkSMProxy* cue = this->Internals->Cue->getProxy();

    if (pqSMAdaptor::getEnumerationProperty(
        cue->GetProperty("TimeMode")) == "Normalized")
      {
      QPair<double, double> range = 
        this->Internals->AnimationScene->getClockTimeRange();
      scaled_time = range.first + ntime*(range.second-range.first);
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
  if (this->Internals->AnimationScene && this->Internals->Cue)
    {
    vtkSMProxy* cue = this->Internals->Cue->getProxy();

    if (pqSMAdaptor::getEnumerationProperty(
        cue->GetProperty("TimeMode")) == "Normalized")
      {
      QPair<double, double> range = 
        this->Internals->AnimationScene->getClockTimeRange();
      if (range.first != range.second)
        {
        time = (time - range.first)/(range.second - range.first);
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
void pqSignalAdaptorKeyFrameTime::setAnimationScene(pqAnimationScene* keeper)
{
  if (this->Internals->AnimationScene)
    {
    QObject::disconnect(this->Internals->AnimationScene, 0, this, 0);
    }
  this->Internals->AnimationScene = keeper;
  if (keeper)
    {
    QObject::connect(keeper, SIGNAL(clockTimeRangesChanged()),
      this, SLOT(timeRangesChanged()));
    }
}
//-----------------------------------------------------------------------------
void pqSignalAdaptorKeyFrameTime::timeRangesChanged()
{
  this->setNormalizedTime(this->Internals->LastTime);
}

