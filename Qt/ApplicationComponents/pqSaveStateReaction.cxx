/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateReaction.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqSaveStateReaction.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPVApplicationCore.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
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
  this->parentAction()->setEnabled(activeObjects->activeServer() != NULL);
}

//-----------------------------------------------------------------------------
bool pqSaveStateReaction::saveState()
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  QString fileExt = tr("ParaView state file (*.pvsm);;Python state file (*.py);;All files (*)");
#else
  QString fileExt = tr("ParaView state file (*.pvsm);;All files (*)");
#endif
  pqFileDialog fileDialog(
    NULL, pqCoreUtilities::mainWidget(), tr("Save State File"), QString(), fileExt);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() == QDialog::Accepted)
  {
    QString selectedFile = fileDialog.getSelectedFiles()[0];
    if (selectedFile.endsWith(".py"))
    {
      pqSaveStateReaction::savePythonState(selectedFile);
    }
    else
    {
      pqSaveStateReaction::saveState(selectedFile);
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::saveState(const QString& filename)
{
  pqApplicationCore::instance()->saveState(filename);
  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename);
}

//-----------------------------------------------------------------------------
void pqSaveStateReaction::savePythonState(const QString& filename)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  vtkSMSessionProxyManager* pxm = pqActiveObjects::instance().proxyManager();
  assert(pxm);

  vtkSmartPointer<vtkSMProxy> options;
  options.TakeReference(pxm->NewProxy("pythontracing", "PythonStateOptions"));
  if (options.GetPointer() == NULL)
  {
    return;
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->InitializeProxy(options);

  pqProxyWidgetDialog dialog(options);
  dialog.setWindowTitle("Python State Options");
  dialog.setObjectName("StateOptionsDialog");
  dialog.setApplyChangesImmediately(true);
  if (dialog.exec() != QDialog::Accepted)
  {
    return;
  }

  vtkStdString state =
    vtkSMTrace::GetState(vtkSMPropertyHelper(options, "PropertiesToTraceOnCreate").GetAsInt(),
      vtkSMPropertyHelper(options, "SkipHiddenDisplayProperties").GetAsInt() == 1);
  if (state.empty())
  {
    qWarning("Empty state generated.");
    return;
  }
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    qWarning() << "Could not open file:" << filename;
    return;
  }
  QTextStream out(&file);
  out << state;
  pqServer* server = pqActiveObjects::instance().activeServer();
  // Add this to the list of recent server resources ...
  pqStandardRecentlyUsedResourceLoaderImplementation::addStateFileToRecentResources(
    server, filename);
#else
  qCritical() << "Failed to save '" << filename
              << "' since Python support in not enabled in this build.";
#endif
}
