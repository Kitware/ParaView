/*=========================================================================

   Program: ParaView
   Module: pqTransferFunctionWidgetPropertyWidget.cxx

   Copyright (c) 2005-2012 Kitware Inc.
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

=========================================================================*/
#include "pqTransferFunctionWidgetPropertyDialog.h"
#include "ui_pqTransferFunctionWidgetPropertyDialog.h"

#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "vtkPiecewiseFunction.h"
#include <QString>

#include <QDoubleValidator>

class pqTransferFunctionWidgetPropertyDialog::pqInternals
{
public:
  Ui::TransferFunctionWidgetPropertyDialog Ui;

  pqInternals(pqTransferFunctionWidgetPropertyDialog* self) { this->Ui.setupUi(self); }
};

pqTransferFunctionWidgetPropertyDialog::pqTransferFunctionWidgetPropertyDialog(const QString& label,
  double* xrange, vtkPiecewiseFunction* transferFunction, QWidget* propertyWdg, QWidget* parentWdg)
  : QDialog(parentWdg)
  , TransferFunction(transferFunction)
  , PropertyWidget(propertyWdg)
  , Internals(new pqTransferFunctionWidgetPropertyDialog::pqInternals(this))
{
  this->setWindowTitle(label);
  this->Internals->Ui.Label->setText(this->Internals->Ui.Label->text().arg(label));
  this->Internals->Ui.TransferFunctionEditor->initialize(
    NULL, false, this->TransferFunction.GetPointer(), true);

  // Make sure the line edits only allow number inputs.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Internals->Ui.minX->setValidator(validator);
  this->Internals->Ui.maxX->setValidator(validator);
  this->Internals->Ui.minX->setText(QString::number(xrange[0]));
  this->Internals->Ui.maxX->setText(QString::number(xrange[1]));

  QObject::connect(this->Internals->Ui.minX, SIGNAL(textChangedAndEditingFinished()), this,
    SLOT(onRangeChanged()));
  QObject::connect(this->Internals->Ui.maxX, SIGNAL(textChangedAndEditingFinished()), this,
    SLOT(onRangeChanged()));
  QObject::connect(this->Internals->Ui.TransferFunctionEditor, SIGNAL(controlPointsModified()),
    this->PropertyWidget, SLOT(propagateProxyPointsProperty()));
  QObject::connect(this->PropertyWidget, SIGNAL(domainChanged()), this, SLOT(onDomainChanged()));
}

pqTransferFunctionWidgetPropertyDialog::~pqTransferFunctionWidgetPropertyDialog()
{
}

void pqTransferFunctionWidgetPropertyDialog::onDomainChanged()
{
  pqTransferFunctionWidgetPropertyWidget* propertyWdg =
    qobject_cast<pqTransferFunctionWidgetPropertyWidget*>(this->PropertyWidget);
  const double* range = propertyWdg->getRange();
  this->Internals->Ui.minX->setText(QString::number(range[0]));
  this->Internals->Ui.maxX->setText(QString::number(range[1]));
}

void pqTransferFunctionWidgetPropertyDialog::onRangeChanged()
{
  pqTransferFunctionWidgetPropertyWidget* propertyWdg =
    qobject_cast<pqTransferFunctionWidgetPropertyWidget*>(this->PropertyWidget);
  propertyWdg->setRange(
    this->Internals->Ui.minX->text().toDouble(), this->Internals->Ui.maxX->text().toDouble());
}
