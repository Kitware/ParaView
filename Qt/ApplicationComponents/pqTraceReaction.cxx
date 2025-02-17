// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTraceReaction.h"

#include "pqActiveObjects.h"
#include "pqAutoSaveBehavior.h"
#include "pqCoreUtilities.h"
#include "pqPVApplicationCore.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <cassert>

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonScriptEditor.h"
#else
class pqPythonScriptEditor : public QObject
{
private:
  Q_DISABLE_COPY(pqPythonScriptEditor);
};
#endif

#include <QMainWindow>
#include <QStatusBar>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqTraceReaction::pqTraceReaction(
  QAction* parentObject, QString start_trace_label, QString stop_trace_label)
  : Superclass(parentObject)
  , StartTraceLabel(start_trace_label)
  , StopTraceLabel(stop_trace_label)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  this->parentAction()->setEnabled(true);
  this->parentAction()->setText(
    vtkSMTrace::GetActiveTracer() == nullptr ? this->StartTraceLabel : this->StopTraceLabel);
#else
  this->parentAction()->setEnabled(false);
  this->parentAction()->setToolTip(
    tr("Tracing unavailable since application built without Python support."));
  this->parentAction()->setStatusTip(
    tr("Tracing unavailable since application built without Python support."));
#endif
}

//-----------------------------------------------------------------------------
pqTraceReaction::~pqTraceReaction()
{
  // ensure to stop trace before the application quits
  if (vtkSMTrace::GetActiveTracer())
  {
    vtkSMTrace::StopTrace();
  }
}

//-----------------------------------------------------------------------------
void pqTraceReaction::onTriggered()
{
  if (vtkSMTrace::GetActiveTracer() == nullptr)
  {
    this->start();
  }
  else
  {
    this->stop();
  }
  this->parentAction()->setText(
    vtkSMTrace::GetActiveTracer() == nullptr ? this->StartTraceLabel : this->StopTraceLabel);
}

//-----------------------------------------------------------------------------
void pqTraceReaction::start()
{
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().activeServer()->proxyManager();

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("pythontracing", "PythonTraceOptions"));
  if (proxy)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->InitializeProxy(proxy);
    pqProxyWidgetDialog dialog(proxy);
    dialog.setWindowTitle(tr("Trace Options"));
    dialog.setObjectName("TraceOptionsDialog");
    dialog.setApplyChangesImmediately(true);
    if (dialog.exec() != QDialog::Accepted)
    {
      return;
    }
  }

  this->AutoSavePythonEnabled = pqAutoSaveBehavior::autoSaveSettingEnabled() &&
    pqAutoSaveBehavior::getStateFormat() == pqApplicationCore::StateFileFormat::Python;
  if (this->AutoSavePythonEnabled)
  {
    auto userAnswer = QMessageBox::warning(pqCoreUtilities::mainWidget(),
      tr("Trace and Auto Save incompatibility."),
      tr("Auto Save Python State setting can not work while Trace is active. Auto Save will be "
         "disabled until the Trace is stopped."),
      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    if (userAnswer == QMessageBox::Cancel)
    {
      return;
    }

    pqAutoSaveBehavior::setAutoSaveSetting(false);
  }

  vtkSMTrace* trace = vtkSMTrace::StartTrace();
  if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget()))
  {
    mainWindow->statusBar()->showMessage(tr("Recording python trace..."));
  }
  if (proxy && trace)
  {
    trace->SetPropertiesToTraceOnCreate(
      vtkSMPropertyHelper(proxy, "PropertiesToTraceOnCreate").GetAsInt());
    trace->SetFullyTraceSupplementalProxies(
      vtkSMPropertyHelper(proxy, "FullyTraceSupplementalProxies").GetAsInt() == 1);
    trace->SetSkipRenderingComponents(
      vtkSMPropertyHelper(proxy, "SkipRenderingComponents").GetAsInt() == 1);
    if (vtkSMPropertyHelper(proxy, "ShowIncrementalTrace").GetAsInt() == 1)
    {
      pqCoreUtilities::connect(trace, vtkCommand::UpdateEvent, this, SLOT(updateTrace()));
    }
    trace->SetFullyTraceCameraAdjustments(
      vtkSMPropertyHelper(proxy, "FullyTraceCameraAdjustments").GetAsInt() == 1);
  }
}

//-----------------------------------------------------------------------------
void pqTraceReaction::stop()
{
  if (this->AutoSavePythonEnabled)
  {
    // re-enable Auto Save State
    pqAutoSaveBehavior::setAutoSaveSetting(true);
  }

  if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget()))
  {
    mainWindow->statusBar()->clearMessage();
  }
  QString tracetxt(vtkSMTrace::StopTrace().c_str());
  this->editTrace(tracetxt, false);
}

//-----------------------------------------------------------------------------
void pqTraceReaction::updateTrace()
{
  this->editTrace(vtkSMTrace::GetActiveTracer()->GetCurrentTrace().c_str(), true);
}

//-----------------------------------------------------------------------------
void pqTraceReaction::editTrace(const QString& trace, bool incremental)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  pqPythonScriptEditor* editor = pqPythonScriptEditor::getUniqueInstance();

  editor->setPythonManager(pqPVApplicationCore::instance()->pythonManager());
  editor->show();

  // Scroll to bottom of the editor when addding content in an incremental trace
  if (incremental)
  {
    editor->updateTrace(trace);
    editor->scrollToBottom();
  }
  else
  {
    editor->stopTrace(trace);
    editor->raise();
    editor->activateWindow();
  }
#endif
}
