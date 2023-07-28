// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqVCRController.h"

// ParaView Server Manager includes.
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMTrace.h"

// Qt includes.
#include <QApplication>
#include <QtDebug>

// ParaView includes.
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqPipelineSource.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
//-----------------------------------------------------------------------------
pqVCRController::pqVCRController(QObject* _parent /*=nullptr*/)
  : QObject(_parent)
{
}

//-----------------------------------------------------------------------------
pqVCRController::~pqVCRController() = default;

//-----------------------------------------------------------------------------
void pqVCRController::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Scene == scene)
  {
    return;
  }
  if (this->Scene)
  {
    QObject::disconnect(this->Scene, nullptr, this, nullptr);
  }
  this->Scene = scene;
  if (this->Scene)
  {
    QObject::connect(this->Scene, SIGNAL(tick(int)), this, SLOT(onTick()));
    QObject::connect(this->Scene, SIGNAL(loopChanged()), this, SLOT(onLoopPropertyChanged()));
    QObject::connect(
      this->Scene, SIGNAL(clockTimeRangesChanged()), this, SLOT(onTimeRangesChanged()));
    QObject::connect(this->Scene, SIGNAL(beginPlay(vtkObject*, unsigned long, void*, void*)), this,
      SLOT(onBeginPlay(vtkObject*, unsigned long, void*, void*)));
    QObject::connect(this->Scene, SIGNAL(endPlay(vtkObject*, unsigned long, void*, void*)), this,
      SLOT(onEndPlay(vtkObject*, unsigned long, void*, void*)));
    bool loop_checked =
      pqSMAdaptor::getElementProperty(scene->getProxy()->GetProperty("Loop")).toBool();
    Q_EMIT this->loop(loop_checked);
  }

  this->onTimeRangesChanged();
  Q_EMIT this->enabled(this->Scene != nullptr);
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqVCRController::getAnimationScene() const
{
  return this->Scene;
}

//-----------------------------------------------------------------------------
void pqVCRController::onTimeRangesChanged()
{
  if (this->Scene)
  {
    QPair<double, double> range = this->Scene->getClockTimeRange();
    Q_EMIT this->timeRanges(range.first, range.second);
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

  CLEAR_UNDO_STACK();
  BEGIN_UNDO_EXCLUDE();

  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("Play");

  this->Scene->getProxy()->InvokeCommand("Play");

  // NOTE: This is a blocking call, returns only after the
  // the animation has stopped.
  END_UNDO_EXCLUDE();

  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqVCRController::onReverse()
{
  if (!this->Scene)
  {
    qDebug() << "No active scene. Cannot play backwards.";
    return;
  }

  CLEAR_UNDO_STACK();
  BEGIN_UNDO_EXCLUDE();

  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("Reverse");

  this->Scene->getProxy()->InvokeCommand("Reverse");

  // NOTE: This is a blocking call, returns only after the
  // the animation has stopped.
  END_UNDO_EXCLUDE();

  pqApplicationCore::instance()->render();
}

//-----------------------------------------------------------------------------
void pqVCRController::onTick()
{
  // No need to explicitly update all views,
  // the animation scene proxy does it.

  // process the events so that the GUI remains responsive.
  QApplication::processEvents();
  Q_EMIT this->timestepChanged();
}

//-----------------------------------------------------------------------------
void pqVCRController::onBeginPlay(vtkObject*, unsigned long, void*, void* reversed)
{
  bool* reversedPtr = reinterpret_cast<bool*>(reversed);
  Q_EMIT this->playing(true, reversedPtr != nullptr ? *reversedPtr : false);
  CLEAR_UNDO_STACK();
  BEGIN_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onEndPlay(vtkObject*, unsigned long, void*, void* reversed)
{
  bool* reversedPtr = reinterpret_cast<bool*>(reversed);
  Q_EMIT this->playing(false, reversedPtr != nullptr ? *reversedPtr : false);
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onLoopPropertyChanged()
{
  vtkSMProxy* scene = this->Scene->getProxy();
  bool loop_checked = pqSMAdaptor::getElementProperty(scene->GetProperty("Loop")).toBool();
  Q_EMIT this->loop(loop_checked);
}

//-----------------------------------------------------------------------------
void pqVCRController::onLoop(bool checked)
{
  BEGIN_UNDO_EXCLUDE();
  vtkSMProxy* scene = this->Scene->getProxy();
  pqSMAdaptor::setElementProperty(scene->GetProperty("Loop"), checked);
  scene->UpdateProperty("Loop");
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onPause()
{
  if (!this->Scene)
  {
    qDebug() << "No active scene. Cannot play.";
    return;
  }
  this->Scene->getProxy()->InvokeCommand("Stop");
}

//-----------------------------------------------------------------------------
void pqVCRController::onFirstFrame()
{
  BEGIN_UNDO_EXCLUDE();
  this->Scene->getProxy()->InvokeCommand("GoToFirst");
  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("GoToFirst");
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onPreviousFrame()
{
  BEGIN_UNDO_EXCLUDE();
  this->Scene->getProxy()->InvokeCommand("GoToPrevious");
  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("GoToPrevious");
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onNextFrame()
{
  BEGIN_UNDO_EXCLUDE();
  this->Scene->getProxy()->InvokeCommand("GoToNext");
  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("GoToNext");
  END_UNDO_EXCLUDE();
}

//-----------------------------------------------------------------------------
void pqVCRController::onLastFrame()
{
  BEGIN_UNDO_EXCLUDE();
  this->Scene->getProxy()->InvokeCommand("GoToLast");
  SM_SCOPED_TRACE(CallMethod).arg(this->Scene->getProxy()).arg("GoToLast");
  END_UNDO_EXCLUDE();
}
