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
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMTrace.h"
#include "vtkSMViewExportHelper.h"
#include "vtkSMViewProxy.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QtDebug>
#include <QToolButton>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
pqExportReaction::pqExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqExportReaction::updateEnableState()
{
  // this results in firing of exportable(bool) signal which updates the
  // QAction's state.
  bool enabled = false;
  if (pqView* view = pqActiveObjects::instance().activeView())
    {
    vtkSMViewProxy* viewProxy = view->getViewProxy();

    vtkNew<vtkSMViewExportHelper> helper;
    enabled = (helper->GetSupportedFileTypes(viewProxy).size() > 0);
    }
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqExportReaction::exportActiveView()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view) { return ;}
  vtkSMViewProxy* viewProxy = view->getViewProxy();

  vtkNew<vtkSMViewExportHelper> helper;
  QString filters(helper->GetSupportedFileTypes(viewProxy).c_str());
  if (filters.isEmpty())
    {
    qCritical("Cannot export current view.");
    return;
    }

  pqFileDialog file_dialog(NULL, pqCoreUtilities::mainWidget(),
    tr("Export View:"), QString(), filters);
  file_dialog.setObjectName("FileExportDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() == QDialog::Accepted &&
    file_dialog.getSelectedFiles().size() > 0)
    {
    QString filename = file_dialog.getSelectedFiles().first();
    vtkSmartPointer<vtkSMExporterProxy> proxy;
    proxy.TakeReference(helper->CreateExporter(filename.toLatin1().data(), viewProxy));
    if (!proxy)
      {
      qCritical("Couldn't handle export filename");
      return;
      }

    QPointer<pqProxyWidget> proxyWidget = new pqProxyWidget(proxy);
    proxyWidget->setApplyChangesImmediately(true);

    // Show a configuration dialog if options are available:
    bool export_cancelled = false;
    if (proxyWidget->filterWidgets(true))
      {
      QDialog dialog(pqCoreUtilities::mainWidget());
      QVBoxLayout *vbox = new QVBoxLayout(&dialog);

      QHBoxLayout *hbox = new QHBoxLayout;

      QLabel *label = new QLabel;
      label->setText(tr("Show advanced options:"));
      hbox->addWidget(label);

      QToolButton *advancedButton = new QToolButton;
      advancedButton->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced26.png"));
      advancedButton->setCheckable(true);
      connect(advancedButton, SIGNAL(toggled(bool)),
        proxyWidget, SLOT(filterWidgets(bool)));
      hbox->addWidget(advancedButton);

      vbox->addLayout(hbox);

      vbox->addWidget(proxyWidget);

      vbox->addStretch();

      QDialogButtonBox *bbox =
        new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
      connect(bbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
      connect(bbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
      vbox->addWidget(bbox);

      dialog.setWindowTitle(tr("Export Options"));

      // While all widgets are shown, fix the size of the dialog. Add a bit to
      // the width, since the default size doesn't leave much room for text in
      // the line edits.
      dialog.adjustSize();
      dialog.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

      // Hide advanced options:
      proxyWidget->filterWidgets();

      // Show the dialog:
      int dialogCode = dialog.exec();
      export_cancelled = (static_cast<QDialog::DialogCode>(dialogCode) != QDialog::Accepted);
      }
    delete proxyWidget;
    if (!export_cancelled)
      {
      SM_SCOPED_TRACE(ExportView)
        .arg("view", viewProxy)
        .arg("exporter", proxy)
        .arg("filename", filename.toLatin1().data());
      proxy->Write();
      }
    }
}
