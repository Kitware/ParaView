// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExportReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqProxyWidget.h"
#include "pqUndoStack.h"
#include "vtkNew.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMViewExportHelper.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtDebug>

#include "pqProxyWidgetDialog.h"

//-----------------------------------------------------------------------------
pqExportReaction::pqExportReaction(QAction* parentObject)
  : Superclass(parentObject)
  , ConnectedView(nullptr)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqExportReaction::updateEnableState()
{
  // this results in firing of exportable(bool) signal which updates the
  // QAction's state.
  pqView* view = pqActiveObjects::instance().activeView();
  if (this->ConnectedView != view)
  {
    if (this->ConnectedView)
    {
      QObject::disconnect(this->ConnectedView,
        SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), this,
        SLOT(updateEnableState()));
    }
    this->ConnectedView = view;
    if (view)
    {
      QObject::connect(this->ConnectedView,
        SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)), this,
        SLOT(updateEnableState()));
    }
  }

  bool enabled = false;
  if (view)
  {
    bool visibleRepresentation = false;
    QList<pqRepresentation*> representations = view->getRepresentations();
    Q_FOREACH (pqRepresentation* repr, representations)
    {
      if (repr->isVisible())
      {
        visibleRepresentation = true;
        break;
      }
    }

    if (visibleRepresentation)
    {
      vtkSMViewProxy* viewProxy = view->getViewProxy();
      vtkNew<vtkSMViewExportHelper> helper;
      enabled = !helper->GetSupportedFileTypes(viewProxy).empty();
    }
  }
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
QString pqExportReaction::exportActiveView()
{
  SCOPED_UNDO_EXCLUDE();

  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    return QString();
  }
  vtkSMViewProxy* viewProxy = view->getViewProxy();

  vtkNew<vtkSMViewExportHelper> helper;
  QString filters(helper->GetSupportedFileTypes(viewProxy).c_str());
  if (filters.isEmpty())
  {
    qCritical("Cannot export current view.");
    return QString();
  }

  pqFileDialog file_dialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Export View:"), QString(), filters, false);
  file_dialog.setObjectName("FileExportDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() == QDialog::Accepted && !file_dialog.getSelectedFiles().empty())
  {
    QString filename = file_dialog.getSelectedFiles().first();
    vtkSmartPointer<vtkSMExporterProxy> proxy;
    proxy.TakeReference(helper->CreateExporter(filename.toUtf8().data(), viewProxy));
    if (!proxy)
    {
      qCritical("Couldn't handle export filename");
      return QString();
    }

    QPointer<pqProxyWidget> proxyWidget = new pqProxyWidget(proxy);
    proxyWidget->setApplyChangesImmediately(true);

    // Show a configuration dialog if options are available:
    bool export_cancelled = false;
    if (proxyWidget->filterWidgets(true))
    {
      pqProxyWidgetDialog dialog(proxy, pqCoreUtilities::mainWidget());
      dialog.setWindowTitle(tr("Export Options"));
      dialog.setApplyChangesImmediately(true);
      int dialogCode = dialog.exec();
      export_cancelled = (static_cast<QDialog::DialogCode>(dialogCode) != QDialog::Accepted);
    }

    delete proxyWidget;
    if (!export_cancelled)
    {
      SM_SCOPED_TRACE(ExportView)
        .arg("view", viewProxy)
        .arg("exporter", proxy)
        .arg("filename", filename.toUtf8().data());
      proxy->Write();
      return filename;
    }
  }
  return QString();
}
