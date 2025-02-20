// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveStateReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <QFile>
#include <QTextStream>
#include <QtDebug>

#include <cassert>

//-----------------------------------------------------------------------------
pqSaveStateReaction::pqSaveStateReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // save state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->parentAction()->setEnabled(activeObjects->activeServer() != nullptr);
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState()
{
  return pqSaveStateReaction::saveState(pqActiveObjects::instance().activeServer());
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState(pqServer* server)
{
  bool pythonAvailable = false;
#if VTK_MODULE_ENABLE_ParaView_pqPython
  pythonAvailable = true;
#endif

  QString fileExt =
    pqApplicationCore::instance()->getDefaultSaveStateFileFormatQString(pythonAvailable, false);

  pqFileDialog fileDialog(
    server, pqCoreUtilities::mainWidget(), tr("Save State File"), QString(), fileExt, false, false);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() == QDialog::Accepted)
  {
    const QString selectedFile = fileDialog.getSelectedFiles()[0];
    if (selectedFile.endsWith(".py"))
    {
      pqSaveStateReaction::savePythonState(selectedFile, fileDialog.getSelectedLocation());
    }
    else
    {
      pqSaveStateReaction::saveState(selectedFile, fileDialog.getSelectedLocation());
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState(const QString& filename, vtkTypeUInt32 location)
{
  SCOPED_UNDO_EXCLUDE();
  if (!pqApplicationCore::instance()->saveState(filename, location))
  {
    qCritical() << tr("Failed to save %1").arg(filename);
    return false;
  }
  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename, location);
  return true;
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::savePythonState(
  const QString& filename, vtkSMProxy* options, vtkTypeUInt32 location)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  SCOPED_UNDO_EXCLUDE();
  if (strcmp(options->GetXMLName(), "PythonStateOptions") != 0)
  {
    qCritical() << tr("Unable to read python state options.");
    return false;
  }

  vtkSMTrace* tracer = vtkSMTrace::GetActiveTracer();
  if (tracer)
  {
    QMessageBox::warning(pqCoreUtilities::mainWidget(),
      tr("Trace and Python State incompatibility."),
      tr("Save Python State can not work while Trace is active. Aborting."));

    return false;
  }

  const std::string state = vtkSMTrace::GetState(options);
  if (state.empty())
  {
    qWarning() << tr("Empty state generated.");
    return false;
  }

  Q_EMIT pqApplicationCore::instance()->aboutToWriteState(filename);

  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  if (!pxm->SaveString(state.c_str(), filename.toStdString().c_str(), location))
  {
    qCritical() << tr("Failed to save state in %1").arg(filename);
    return false;
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename, location);

  return true;

#else
  Q_UNUSED(location);
  Q_UNUSED(options);
  qCritical()
    << tr("Failed to save '%1' since Python support is not enabled in this build.").arg(filename);

  return false;
#endif
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqSaveStateReaction::createPythonStateOptions(bool interactive)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  vtkSMProxy* options = pxm->NewProxy("pythontracing", "PythonStateOptions");
  if (options)
  {
    vtkNew<vtkSMParaViewPipelineController> controller;
    controller->InitializeProxy(options);
  }

  if (interactive)
  {
    pqProxyWidgetDialog dialog(options);
    dialog.setWindowTitle(tr("Python State Options"));
    dialog.setObjectName("StateOptionsDialog");
    dialog.setApplyChangesImmediately(false);
    dialog.exec();
  }

  return options;

#else
  Q_UNUSED(interactive);
  qCritical() << tr(
    "Failed to create python state options since Python support is not enabled in this build.");

  return nullptr;
#endif
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::savePythonState(const QString& filename, vtkTypeUInt32 location)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  vtkSmartPointer<vtkSMProxy> options;
  options.TakeReference(pqSaveStateReaction::createPythonStateOptions(true));

  return pqSaveStateReaction::savePythonState(filename, options, location);
#else
  Q_UNUSED(location);
  qCritical()
    << tr("Failed to save '%1' since Python support is not enabled in this build.").arg(filename);
  return false;
#endif
}
