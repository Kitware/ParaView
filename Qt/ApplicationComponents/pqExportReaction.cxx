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
#include "pqPropertiesPanel.h"
#include "pqPropertyWidget.h"
#include "pqViewExporterManager.h"
#include "pqFileDialog.h"

#include "vtkNew.h"
#include "vtkSMExporterProxy.h"
#include "vtkSMOrderedPropertyIterator.h"
#include "vtkSMProperty.h"

#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
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

    // Create widgets for exporter properties
    QList<pqPropertyWidget*> widgets;
    QStringList labels;
    vtkNew<vtkSMOrderedPropertyIterator> propertyIter;
    propertyIter->SetProxy(proxy);

    for (propertyIter->Begin(); !propertyIter->IsAtEnd(); propertyIter->Next())
      {
      vtkSMProperty *smProperty = propertyIter->GetProperty();

      if (smProperty->GetIsInternal() || smProperty->GetInformationOnly() ||
          QString(smProperty->GetPanelVisibility()) == "never")
        {
        continue;
        }

      pqPropertyWidget *propertyWidget =
        pqPropertiesPanel::createWidgetForProperty(smProperty, proxy);

      const char *xmlLabel = smProperty->GetXMLLabel();
      if (propertyWidget)
        {
        // Clear the text from any checkboxes -- otherwise the same text appears
        // on both sides of the check box. We can find out if the property
        // widget contains a checkbox by inspecting its children.
        if (QCheckBox *checkBox = propertyWidget->findChild<QCheckBox*>())
          {
          checkBox->setText(QString());
          }

        widgets << propertyWidget;
        if (xmlLabel)
          {
          labels << xmlLabel;
          }
        else
          {
          labels << propertyIter->GetKey();
          }
        }
      }

    if (labels.size() != widgets.size())
      {
      qWarning("Number of labels does not match number of widgets for export "
               "configuration. Using defaults.");
      labels.clear();
      qDeleteAll(widgets);
      widgets.clear();
      }

    // Show a configuration dialog if options are available:
    if (widgets.size() != 0)
      {
      QDialog dialog;
      QVBoxLayout layout;
      QFormLayout form;
      for (int i = 0; i < widgets.size(); ++i)
        {
        widgets[i]->setParent(&dialog);
        form.addRow(labels[i], widgets[i]);
        }
      layout.addLayout(&form);

      QDialogButtonBox bbox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
      connect(&bbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
      connect(&bbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
      layout.addWidget(&bbox);

      dialog.setLayout(&layout);

      dialog.setWindowTitle(tr("Export Options"));

      // Show the dialog:
      int dialogCode = dialog.exec();

      if (static_cast<QDialog::DialogCode>(dialogCode) == QDialog::Rejected)
        {
        proxy->Delete();
        return;
        }

      foreach (pqPropertyWidget *widget, widgets)
        {
        widget->apply();
        }

      // Widgets are cleaned up by the dialog
      widgets.clear();
      }

    if (!this->Exporter->write(proxy))
      {
      qCritical("Failed to export correctly.");
      }
    }
}
