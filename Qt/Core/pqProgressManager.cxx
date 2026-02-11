// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProgressManager.h"

#include <QApplication>
#include <QEvent>

#include "pqApplicationCore.h"
#include "pqCoreConfiguration.h"
#include "pqCoreUtilities.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkOutputWindow.h"
#include "vtkPVLogger.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMProxy.h"
#include "vtkSMSession.h"
#include "vtkTimerLog.h"

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
{
  this->ProgressCount = 0;
  this->InUpdate = false;

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
void pqProgressManager::setProgress(
  const QString& message, int progress_val, bool processEvents /*=false*/)
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
  vtkVLogScopeF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "setProgress %d", progress_val);
  this->InUpdate = true;
  Q_EMIT this->progress(message, progress_val);
  if (processEvents)
  {
    pqCoreUtilities::processEvents();
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
  pqApplicationCore* appCore = pqApplicationCore::instance();
  if (auto* server = appCore->getActiveServer())
  {
    if (auto* progressHandler = server->session()->GetProgressHandler())
    {
      const auto abortGid = progressHandler->GetLastProgressId();
      if (abortGid > 0)
      {
        if (auto* proxy = vtkSMProxy::SafeDownCast(server->session()->GetRemoteObject(abortGid)))
        {
          if (auto* algorithm = vtkAlgorithm::SafeDownCast(proxy->GetClientSideObject()))
          {
            vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
              "abort gid=" << abortGid << ",object=" << algorithm->GetObjectDescription());
            algorithm->SetAbortExecuteAndUpdateTime();
            pqApplicationCore* core = pqApplicationCore::instance();
            pqServerManagerModel* smModel = core->getServerManagerModel();
            pqProxy* pqproxy = smModel->findItem<pqProxy*>(proxy);
            pqproxy->setModifiedState(pqProxy::ABORTED);
          }
          else
          {
            vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
              "abort triggered, but no vtkAlgorithm found for gid="
                << abortGid << ", found " << proxy->GetClassName() << " instead.");
          }
        }
        else
        {
          vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
            "abort triggered, but no vtkSMProxy found for gid=" << abortGid);
        }
      }
      else
      {
        vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
          "abort triggered, but gid of last progress proxy is 0!");
      }
    }
    else
    {
      vtkLog(ERROR, << "abort triggered with no progress handler.");
    }
  }
  else
  {
    vtkLog(ERROR, << "abort triggered with no active server.");
  }
}

//-----------------------------------------------------------------------------
void pqProgressManager::onStartProgress()
{
  Q_EMIT progressStartEvent();
  this->setEnableProgress(true);
  this->setEnableAbort(true);
  QApplication::instance()->installEventFilter(this);
}

//-----------------------------------------------------------------------------
void pqProgressManager::onEndProgress()
{
  QApplication::instance()->removeEventFilter(this);
  this->setEnableProgress(false);
  this->setEnableAbort(false);
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

  // Do not call processEvents when running tests. Doing so can cause
  // tests to fail due to tag mismatch problems. It may be because of a
  // conflict with the processEvents() invoked inside pqTestUtility::playTests().
  bool processEvents = pqCoreConfiguration::instance()->testScriptCount() == 0;
  // we don't want to call a processEvents on zero progress
  // since that breaks numerous other classes currently in ParaView
  // mainly because of subtle timing issues from QTimers that are expected
  // to expire in a certain order
  processEvents &= (oldProgress > 0);
  // must be builtin to safely process events.
  if (auto* server = pqApplicationCore::instance()->getActiveServer())
  {
    // processEvents &= session->HasProcessRole(vtkPVSession::CLIENT_AND_SERVERS);
    processEvents &= server->isRemote() ? false : true;
  }
  this->setProgress(text, oldProgress, processEvents);
}
