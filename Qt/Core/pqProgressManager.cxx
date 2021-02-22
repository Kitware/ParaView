/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.cxx

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

========================================================================*/
#include "pqProgressManager.h"

#include <QApplication>
#include <QEvent>

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkCommand.h"
#include "vtkOutputWindow.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
{
  this->ProgressCount = 0;
  this->InUpdate = false;
  QApplication::instance()->installEventFilter(this);

  this->EnableProgress = false;
  this->UnblockEvents = false;
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerAdded(pqServer*)));
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager() = default;

//-----------------------------------------------------------------------------
void pqProgressManager::onServerAdded(pqServer* server)
{
  vtkPVProgressHandler* progressHandler = server->session()->GetProgressHandler();

  pqCoreUtilities::connect(progressHandler, vtkCommand::StartEvent, this, SLOT(onStartProgress()));
  pqCoreUtilities::connect(progressHandler, vtkCommand::EndEvent, this, SLOT(onEndProgress()));
  pqCoreUtilities::connect(
    progressHandler, vtkCommand::ProgressEvent, this, SLOT(onProgress(vtkObject*)));
}

//-----------------------------------------------------------------------------
bool pqProgressManager::eventFilter(QObject* obj, QEvent* evt)
{
  bool skipEvent = false;
  bool skippableEvent = evt->type() == QEvent::KeyPress || evt->type() == QEvent::MouseButtonPress;
  if (this->ProgressCount > 0 && skippableEvent && !this->UnblockEvents)
  {
    skipEvent = (this->NonBlockableObjects.contains(obj) == false);
  }
  return skipEvent ? true : this->QObject::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
bool pqProgressManager::unblockEvents(bool val)
{
  bool prev = this->UnblockEvents;
  this->UnblockEvents = val;
  return prev;
}

//-----------------------------------------------------------------------------
void pqProgressManager::lockProgress(QObject* object)
{
  if (!object)
  {
    return;
  }

  if (this->Lock)
  {
    return;
  }
  this->Lock = object;
}

//-----------------------------------------------------------------------------
void pqProgressManager::unlockProgress(QObject* object)
{
  if (!object)
  {
    return;
  }

  if (this->Lock == object)
  {
    this->Lock = nullptr;
  }
}

//-----------------------------------------------------------------------------
bool pqProgressManager::isLocked() const
{
  return (this->Lock != nullptr);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setProgress(const QString& message, int progress_val)
{
  if (this->Lock && this->Lock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }
  if (this->InUpdate)
  {
    return;
  }
  this->InUpdate = true;
  Q_EMIT this->progress(message, progress_val);
  if (progress_val > 0)
  {
    // we don't want to call a processEvents on zero progress
    // since that breaks numerous other classes currently in ParaView
    // mainly because of subtle timing issues from QTimers that are expected
    // to expire in a certain order

    // we are disabling this for the 3.12 release so we are sure we don't
    // get tag mismatches in the release product
    // pqCoreUtilities::processEvents(QEventLoop::ExcludeUserInputEvents);
  }
  this->InUpdate = false;
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableAbort(bool enable)
{
  if (this->Lock && this->Lock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }
  Q_EMIT this->enableAbort(enable);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableProgress(bool enable)
{
  if (this->Lock && this->Lock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }

  if (enable)
  {
    if (this->ProgressCount++ == 0)
    {
      this->EnableProgress = true;
      Q_EMIT this->enableProgress(true);
    }
  }
  else
  {
    if (--this->ProgressCount == 0)
    {
      this->EnableProgress = false;
      Q_EMIT this->enableProgress(false);
    }
  }
}

//-----------------------------------------------------------------------------
void pqProgressManager::triggerAbort()
{
  Q_EMIT this->abort();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onStartProgress()
{
  Q_EMIT progressStartEvent();
  this->setEnableProgress(true);
}

//-----------------------------------------------------------------------------
void pqProgressManager::onEndProgress()
{
  this->setEnableProgress(false);
  Q_EMIT progressEndEvent();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onProgress(vtkObject* caller)
{
  vtkPVProgressHandler* handler = vtkPVProgressHandler::SafeDownCast(caller);
  int oldProgress = handler->GetLastProgress();
  QString text = handler->GetLastProgressText();

  if (!this->EnableProgress)
  {
    return;
  }

  // chop of "vtk" prefix
  if (text.startsWith("vtk"))
  {
    text = text.mid(3);
  }
  this->setProgress(text, oldProgress);
}
