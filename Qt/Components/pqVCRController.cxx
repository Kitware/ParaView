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
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMIntRangeDomain.h"

// Qt includes.
#include <QtDebug>
#include <QCoreApplication>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderViewModule.h"
#include "pqUndoStack.h"
#include "pqAnimationScene.h"
#include "pqSMAdaptor.h"
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
    QObject::connect(this->Scene, SIGNAL(tick()), this, SLOT(onTick()));
    }

  emit this->enabled (this->Scene != NULL);
}

//-----------------------------------------------------------------------------
void pqVCRController::onPlay()
{
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot play.";
    return;
    }

  emit this->playing(true);
 
  vtkSMAnimationSceneProxy* scene = this->Scene->getAnimationSceneProxy();
  scene->Play(); // NOTE: This is a blocking call, returns only after the
                 // the animation has stopped.

  emit this->playing(false);

  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqVCRController::onTick()
{
  // update all views.
  pqApplicationCore::instance()->render();

  // process the events so that the GUI remains responsive.
  QCoreApplication::processEvents();

  emit this->timestepChanged();
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
  this->updateScene(true, false, 0);
}

//-----------------------------------------------------------------------------
void pqVCRController::onPreviousFrame()
{
  this->updateScene(false, false, -1);
}

//-----------------------------------------------------------------------------
void pqVCRController::onNextFrame()
{
  this->updateScene(false, false, 1);
}

//-----------------------------------------------------------------------------
void pqVCRController::onLastFrame()
{
  this->updateScene(false, true, 0);
}

//-----------------------------------------------------------------------------
bool pqVCRController::updateScene(bool first, bool last, int offset)
{
  // TODO: We may want to move this "next" logic to vtkSMAnimationSceneProxy
  // itself (or even vtkSMAnimationScene). This will need to take into consideration
  // the new "ENUMERATED_TIMES" mode as well.
  if (!this->Scene)
    {
    qDebug() << "No active scene. Cannot go to first frame.";
    return false;
    }

  vtkSMAnimationSceneProxy* scene = this->Scene->getAnimationSceneProxy();
  double start_time = pqSMAdaptor::getElementProperty(
    scene->GetProperty("StartTime")).toDouble();
  double end_time = pqSMAdaptor::getElementProperty(
    scene->GetProperty("EndTime")).toDouble();
  double new_time = pqSMAdaptor::getElementProperty(
    scene->GetProperty("CurrentTime")).toDouble();

  if (first)
    {
    new_time = start_time;
    }
  else if (last)
    {
    new_time = end_time;
    }
  else 
    {
    new_time += offset;
    new_time = (new_time < start_time)? start_time : new_time;
    new_time = (new_time > end_time)? end_time : new_time;
    }
  pqSMAdaptor::setElementProperty(scene->GetProperty("CurrentTime"), new_time);
  scene->UpdateProperty("CurrentTime", 1);

  this->onTick();
  return true;
}


