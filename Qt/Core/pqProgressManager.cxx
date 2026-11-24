// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqProgressManager.h"

#include <QApplication>
#include <QEvent>
#include <QMainWindow>

#include "pqApplicationCore.h"
#include "pqCoreConfiguration.h"
#include "pqCoreUtilities.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkAlgorithm.h"
#include "vtkCommand.h"
#include "vtkOutputWindow.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVLogger.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkTimerLog.h"

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
{
  this->ProgressCount = 0;
  this->InUpdate = false;

  this->EnableProgress = false;
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerAdded(pqServer*)));
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager() = default;

//-----------------------------------------------------------------------------
void pqProgressManager::onServerAdded(pqServer* server)
{
  vtkPVProgressHandler* progressHandler = server->session()->GetProgressHandler();

  Qt::ConnectionType coType = Qt::AutoConnection;
  if (vtkPVGeneralSettings::GetInstance()->GetEnableThreadedUpdate())
  {
    // `Qt::QueuedConnection` matters here, in order to go through the thread barrier
    coType = Qt::QueuedConnection;
  }

  pqCoreUtilities::connect(
    progressHandler, vtkCommand::StartEvent, this, SLOT(onStartProgress()), coType);
  pqCoreUtilities::connect(
    progressHandler, vtkCommand::EndEvent, this, SLOT(onEndProgress()), coType);
  pqCoreUtilities::connect(
    progressHandler, vtkCommand::ProgressEvent, this, SLOT(onProgress(vtkObject*)), coType);
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
  if (processEvents && this->AbortEnabled)
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

  if (enable)
  {
    // Disable everything that is enabled, except non blockable objects
    QMainWindow* win = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
    QList<QWidget*> widgets = win->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    this->DisabledWidgetsForAbort.clear();

    // Disabling widgets makes the focused widget lose focus, remember it to restore it later
    this->FocusWidgetForAbort = QApplication::focusWidget();
    for (QWidget* widget : widgets)
    {
      if (widget->isEnabled() && this->NonBlockableObjects.contains(widget) == false)
      {
        this->DisabledWidgetsForAbort.push_front(widget);
      }
    }
    for (const auto& widget : this->DisabledWidgetsForAbort)
    {
      if (widget)
      {
        widget->setDisabled(true);
      }
    }
  }
  else
  {
    // Reenable everything that was disabled
    for (const auto& widget : this->DisabledWidgetsForAbort)
    {
      // Applying changes can destroy widgets while processing events.
      if (widget)
      {
        widget->setEnabled(true);
      }
    }
    this->DisabledWidgetsForAbort.clear();

    // Restore focus lost when disabling widgets, eg. the properties panel search box
    // when using auto apply.
    if (this->FocusWidgetForAbort && this->FocusWidgetForAbort->isEnabled() &&
      this->FocusWidgetForAbort->isVisible() &&
      QApplication::focusWidget() != this->FocusWidgetForAbort)
    {
      this->FocusWidgetForAbort->setFocus(Qt::OtherFocusReason);
    }
    this->FocusWidgetForAbort = nullptr;
  }

  Q_EMIT this->enableAbort(enable);
  this->AbortEnabled = enable;
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
  if (pqServer* server = appCore->getActiveServer())
  {
    if (vtkPVProgressHandler* progressHandler = server->session()->GetProgressHandler())
    {
      const auto abortGid = progressHandler->GetLastProgressId();
      if (abortGid > 0)
      {
        if (vtkSMSourceProxy::SafeDownCast(server->session()->GetRemoteObject(abortGid)))
        {
          // Only abort if we find the proxy client side
          progressHandler->Abort(abortGid);
        }
        else
        {
          vtkVLog(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
            "abort triggered, but no vtkSMSourceProxy found for gid=" << abortGid);
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

  // Do not call processEvents when running tests. Doing so can cause
  // tests to fail due to tag mismatch problems. It may be because of a
  // conflict with the processEvents() invoked inside pqTestUtility::playTests().
  bool processEvents = pqCoreConfiguration::instance()->testScriptCount() == 0;
  // we don't want to call a processEvents on zero progress
  // since that breaks numerous other classes currently in ParaView
  // mainly because of subtle timing issues from QTimers that are expected
  // to expire in a certain order
  processEvents &= (oldProgress > 0);
  this->setProgress(text, oldProgress, processEvents);
}
