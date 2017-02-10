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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOutputWindow.h"
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
  this->ReadyEnableProgress = false;
  this->LastProgressTime = 0;
  this->UnblockEvents = false;

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerAdded(pqServer*)));
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager()
{
}

//-----------------------------------------------------------------------------
void pqProgressManager::onServerAdded(pqServer* server)
{
  vtkPVProgressHandler* progressHandler = server->session()->GetProgressHandler();

  pqCoreUtilities::connect(progressHandler, vtkCommand::StartEvent, this, SLOT(onStartProgress()));
  pqCoreUtilities::connect(progressHandler, vtkCommand::EndEvent, this, SLOT(onEndProgress()));
  pqCoreUtilities::connect(
    progressHandler, vtkCommand::ProgressEvent, this, SLOT(onProgress(vtkObject*)));
  pqCoreUtilities::connect(
    progressHandler, vtkCommand::MessageEvent, this, SLOT(onMessage(vtkObject*)));
}

//-----------------------------------------------------------------------------
bool pqProgressManager::eventFilter(QObject* obj, QEvent* evt)
{
  if (this->ProgressCount != 0 && !this->UnblockEvents)
  {
    if (dynamic_cast<QKeyEvent*>(evt) || dynamic_cast<QMouseEvent*>(evt))
    {
      if (!this->NonBlockableObjects.contains(obj))
      {
        return true;
      }
    }
  }

  return QObject::eventFilter(obj, evt);
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
    this->Lock = 0;
  }
}

//-----------------------------------------------------------------------------
bool pqProgressManager::isLocked() const
{
  return (this->Lock != 0);
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
  emit this->progress(message, progress_val);
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
  emit this->enableAbort(enable);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableProgress(bool enable)
{
  if (this->Lock && this->Lock != this->sender())
  {
    // When locked, ignore all other senders.
    return;
  }

  this->ProgressCount += (enable ? 1 : -1);
  if (this->ProgressCount < 0)
  {
    this->ProgressCount = 0;
  }

  if (this->InUpdate)
  {
    return;
  }
  this->InUpdate = true;
  if (this->ProgressCount <= 1)
  {
    emit this->enableProgress(enable);
  }
  this->InUpdate = false;
}

//-----------------------------------------------------------------------------
void pqProgressManager::triggerAbort()
{
  emit this->abort();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onStartProgress()
{
  this->ReadyEnableProgress = true;
  emit progressStartEvent();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onEndProgress()
{
  this->ReadyEnableProgress = false;
  if (this->EnableProgress)
  {
    this->setEnableProgress(false);
  }
  this->EnableProgress = false;
  emit progressEndEvent();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onProgress(vtkObject* caller)
{
  vtkPVProgressHandler* handler = vtkPVProgressHandler::SafeDownCast(caller);
  int oldProgress = handler->GetLastProgress();
  QString text = handler->GetLastProgressText();

  if (this->ReadyEnableProgress == false)
  {
    return;
  }

  // only forward progress events to the GUI if we get at least .05 seconds
  // since the last time we forwarded the progress event
  double lastprog = vtkTimerLog::GetUniversalTime();
  if (lastprog - this->LastProgressTime < .05)
  {
    return;
  }

  // We will show progress. Reset timer.
  this->LastProgressTime = vtkTimerLog::GetUniversalTime();

  // delayed progress starting so the progress bar doesn't flicker
  // so much for the quick operations
  if (this->EnableProgress == false)
  {
    this->EnableProgress = true;
    this->setEnableProgress(true);
  }

  this->LastProgressTime = lastprog;

  // chop of "vtk" prefix
  if (text.startsWith("vtk"))
  {
    text = text.mid(3);
  }
  this->setProgress(text, oldProgress);
}

//-----------------------------------------------------------------------------
void pqProgressManager::onMessage(vtkObject* caller)
{
  vtkPVProgressHandler* handler = vtkPVProgressHandler::SafeDownCast(caller);
  QString text = handler->GetLastMessage();
  if (text.startsWith("ERROR: "))
  {
    vtkOutputWindow::GetInstance()->DisplayErrorText(text.toStdString().c_str());
  }
  else if (text.startsWith("Warning: "))
  {
    vtkOutputWindow::GetInstance()->DisplayWarningText(text.toStdString().c_str());
  }
  else if (text.startsWith("Generic Warning: "))
  {
    vtkOutputWindow::GetInstance()->DisplayGenericWarningText(text.toStdString().c_str());
  }
  else if (text.startsWith("Debug : "))
  {
    vtkOutputWindow::GetInstance()->DisplayText(text.toStdString().c_str());
  }
  else
  {
    vtkOutputWindow::GetInstance()->DisplayText(text.toStdString().c_str());
  }
}
