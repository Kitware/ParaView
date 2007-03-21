/*=========================================================================

   Program: ParaView
   Module:    pqVCRController.cxx

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
#include "pqVCRController.h"

// ParaView Server Manager includes.
#include "vtkSMPVAnimationSceneProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIntRangeDomain.h"

// Qt includes.
#include <QtDebug>
#include <QCoreApplication>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqAnimationScene.h"
#include "pqSMAdaptor.h"
#include "pqEventDispatcher.h"
//-----------------------------------------------------------------------------
pqVCRController::pqVCRController(QObject* _parent/*=null*/) : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqVCRController::~pqVCRController()
{
}

//-----------------------------------------------------------------------------
void pqVCRController::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Scene == scene)
    {
    return;
    }
  if (this->Scene)
    {
    QObject::disconnect(this->Scene, 0, this, 0);
    }
  this->Scene = scene;
  if (this->Scene)
    {
    QObject::connect(this->Scene, SIGNAL(tick(int)), this, SLOT(onTick()));
    QObject::connect(this->Scene, SIGNAL(loopChanged()),
      this, SLOT(onLoopPropertyChanged()));
    QObject::connect(this->Scene, SIGNAL(clockTimeRangesChanged()),
        this, SLOT(onTimeRangesChanged()));
    QObject::connect(this->Scene, SIGNAL(beginPlay()),
      this, SLOT(onBeginPlay()));
    QObject::connect(this->Scene, SIGNAL(endPlay()),
      this, SLOT(onEndPlay()));
    bool loop_checked = pqSMAdaptor::getElementProperty(
        scene->getProxy()->GetProperty("Loop")).toBool();
    emit this->loop(loop_checked);
    }

  this->onTimeRangesChanged();
  emit this->enabled (this->Scene != NULL);
}

//-----------------------------------------------------------------------------
void pqVCRController::onTimeRangesChanged()
{
  if (this->Scene)
    {
    QPair<double, double> range = this->Scene->getClockTimeRange();
    emit this->timeRanges(range.first, range.second);
    }
}

//-----------------------------------------------------------------------------
void pqVCRController::onPlay()
{
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot play.";
    return;
    }

  vtkSMAnimationSceneProxy* scene = this->Scene->getAnimationSceneProxy();
  scene->Play(); // NOTE: This is a blocking call, returns only after the
                 // the animation has stopped.

  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqVCRController::onTick()
{
  // No need to explicitly update all views,
  // the animation scene proxy does it.

  // process the events so that the GUI remains responsive.
  pqEventDispatcher::processEventsAndWait(1);  // would be nice to enhance
                                               // testing framework so we can
                                               // simply call
                                               // QApplication::processEvents

  emit this->timestepChanged();
}

//-----------------------------------------------------------------------------
void pqVCRController::onBeginPlay()
{
  emit this->playing(true);
  emit this->beginNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqVCRController::onEndPlay()
{
  emit this->playing(false);
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqVCRController::onLoopPropertyChanged()
{
  vtkSMProxy* scene = this->Scene->getProxy();
  bool loop_checked = pqSMAdaptor::getElementProperty(
    scene->GetProperty("Loop")).toBool();
  emit this->loop(loop_checked);
}

//-----------------------------------------------------------------------------
void pqVCRController::onLoop(bool checked)
{
  vtkSMAnimationSceneProxy* scene = this->Scene->getAnimationSceneProxy();
  pqSMAdaptor::setElementProperty(scene->GetProperty("Loop"),checked);
  scene->UpdateProperty("Loop");
}

//-----------------------------------------------------------------------------
void pqVCRController::onPause()
{
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot play.";
    return;
    }
  vtkSMAnimationSceneProxy* scene = this->Scene->getAnimationSceneProxy();
  scene->Stop();
}
  
//-----------------------------------------------------------------------------
void pqVCRController::onFirstFrame()
{
  emit this->beginNonUndoableChanges();
  vtkSMPVAnimationSceneProxy* scene = vtkSMPVAnimationSceneProxy::SafeDownCast(
    this->Scene->getProxy());
  if (scene)
    {
    scene->GoToFirst();
    }
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqVCRController::onPreviousFrame()
{
  emit this->beginNonUndoableChanges();
  vtkSMPVAnimationSceneProxy* scene = vtkSMPVAnimationSceneProxy::SafeDownCast(
    this->Scene->getProxy());
  if (scene)
    {
    scene->GoToPrevious();
    }
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqVCRController::onNextFrame()
{
  emit this->beginNonUndoableChanges();
  vtkSMPVAnimationSceneProxy* scene = vtkSMPVAnimationSceneProxy::SafeDownCast(
    this->Scene->getProxy());
  if (scene)
    {
    scene->GoToNext();
    }
  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqVCRController::onLastFrame()
{
  emit this->beginNonUndoableChanges();
  vtkSMPVAnimationSceneProxy* scene = vtkSMPVAnimationSceneProxy::SafeDownCast(
    this->Scene->getProxy());
  if (scene)
    {
    scene->GoToLast();
    }
  emit this->endNonUndoableChanges();
}

