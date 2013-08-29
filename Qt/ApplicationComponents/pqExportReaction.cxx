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
#include "pqViewExporterManager.h"

#include "vtkSMExporterProxy.h"
#include "vtkSMProperty.h"

#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

//-----------------------------------------------------------------------------
pqExportReaction::pqExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->Exporter = new pqViewExporterManager(this);

  QObject::connect(this->Exporter, SIGNAL(exportable(bool)),
    parentObject, SLOT(setEnabled(bool)));

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
  this->Exporter->setView(pqActiveObjects::instance().activeView());
}

//-----------------------------------------------------------------------------
void pqExportReaction::exportActiveView()
{
  QString filters = this->Exporter->getSupportedFileTypes();
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
    vtkSMExporterProxy *proxy = this->Exporter->proxyForFile(
      file_dialog.getSelectedFiles().first());

    if (!proxy)
      {
      qCritical("Couldn't handle export filename.");
      return;
      }

    QPointer<pqProxyWidget> proxyWidget = new pqProxyWidget(proxy);
    proxyWidget->setApplyChangesImmediately(true);

    // Show a configuration dialog if options are available:
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

      if (static_cast<QDialog::DialogCode>(dialogCode) == QDialog::Rejected)
        {
        proxy->Delete();
        return;
        }
      }

    delete proxyWidget;

    if (!this->Exporter->write(proxy))
      {
      qCritical("Failed to export correctly.");
      }
    }
}
