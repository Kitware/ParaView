// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTransferFunctionWidgetPropertyDialog.h"
#include "ui_pqTransferFunctionWidgetPropertyDialog.h"

#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "vtkPiecewiseFunction.h"

#include <QAction>
#include <QDoubleValidator>
#include <QString>
#include <QStyle>

class pqTransferFunctionWidgetPropertyDialog::pqInternals
{
public:
  Ui::TransferFunctionWidgetPropertyDialog Ui;

  pqInternals(pqTransferFunctionWidgetPropertyDialog* self) { this->Ui.setupUi(self); }
};

pqTransferFunctionWidgetPropertyDialog::pqTransferFunctionWidgetPropertyDialog(const QString& label,
  vtkPiecewiseFunction* transferFunction, QWidget* propertyWdg, QWidget* parentWdg)
  : QDialog(parentWdg)
  , TransferFunction(transferFunction)
  , PropertyWidget(propertyWdg)
  , Internals(new pqTransferFunctionWidgetPropertyDialog::pqInternals(this))
{
  this->setWindowTitle(label);
  this->Internals->Ui.Label->setText(this->Internals->Ui.Label->text().arg(label));
  this->Internals->Ui.TransferFunctionEditor->initialize(
    nullptr, false, this->TransferFunction.GetPointer(), true);

  QObject::connect(this->Internals->Ui.TransferFunctionEditor, SIGNAL(controlPointsModified()),
    this->PropertyWidget, SLOT(propagateProxyPointsProperty()));

  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));
}

pqTransferFunctionWidgetPropertyDialog::~pqTransferFunctionWidgetPropertyDialog() = default;
