/*=========================================================================

   Program: ParaView
   Module:    pqExportReaction.cxx

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
    nullptr, pqCoreUtilities::mainWidget(), tr("Export View:"), QString(), filters);
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
      dialog.setWindowTitle("Export Options");
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
