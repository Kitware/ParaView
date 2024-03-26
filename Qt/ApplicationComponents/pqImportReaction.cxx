// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqImportReaction.h"
#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidget.h"
#include "pqProxyWidgetDialog.h"
#include "pqRenderView.h"

#include "vtkSMImporterFactory.h"
#include "vtkSMImporterProxy.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTrace.h"
#include "vtkSMViewProxy.h"

//----------------------------------------------------------------------------
pqImportReaction::pqImportReaction(QAction* parent)
  : Superclass(parent)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//----------------------------------------------------------------------------
void pqImportReaction::import()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }
  const QString filters(vtkSMImporterFactory::GetSupportedFileTypes(server->session()).c_str());
  if (filters.isEmpty())
  {
    qCritical("Cannot import to current session");
    return;
  }

  pqFileDialog file_dialog(
    server, pqCoreUtilities::mainWidget(), tr("Import:"), QString(), filters);
  file_dialog.setObjectName("FileImportDialog");
  file_dialog.setFileMode(pqFileDialog::ExistingFile);
  if (file_dialog.exec() == QDialog::Accepted && !file_dialog.getSelectedFiles().empty())
  {
    const QString filename = file_dialog.getSelectedFiles().first();
    vtkSmartPointer<vtkSMImporterProxy> proxy;
    proxy.TakeReference(
      vtkSMImporterFactory::CreateImporter(filename.toUtf8().data(), server->session()));
    proxy->UpdatePipelineInformation();
    if (!proxy)
    {
      qCritical("Couldn't import filename %s", filename.toStdString().c_str());
      return;
    }

    QPointer<pqProxyWidget> const proxyWidget = new pqProxyWidget(proxy);
    proxyWidget->setApplyChangesImmediately(true);

    // Show a configuration dialog if options are available:
    bool import_cancelled = false;
    if (proxyWidget->filterWidgets(true))
    {
      pqProxyWidgetDialog dialog(proxy, pqCoreUtilities::mainWidget());
      dialog.setWindowTitle(tr("Import Options"));
      dialog.setApplyChangesImmediately(true);
      const int statusCode = dialog.exec();
      import_cancelled = (static_cast<QDialog::DialogCode>(statusCode) != QDialog::Accepted);
    }

    delete proxyWidget;
    if (!import_cancelled)
    {
      pqRenderView* renderView =
        qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
      SM_SCOPED_TRACE(ImportView)
        .arg("view", renderView->getViewProxy())
        .arg("importer", proxy.Get())
        .arg("filename", filename.toUtf8().data());
      proxy->Import(renderView->getRenderViewProxy());
      renderView->render();
    }
    else
    {
      server->session()->GetSessionProxyManager()->UnRegisterProxy(proxy);
    }
  }
}

//----------------------------------------------------------------------------
void pqImportReaction::updateEnableState()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  this->parentAction()->setEnabled(server != nullptr && renderView != nullptr);
}
