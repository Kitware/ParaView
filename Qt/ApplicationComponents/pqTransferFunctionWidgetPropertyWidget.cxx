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
#include "pqTransferFunctionWidgetPropertyWidget.h"
#include "ui_pqTransferFunctionWidgetPropertyWidget.h"

#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqPVApplicationCore.h"
#include "pqTransferFunctionWidget.h"
#include "pqTransferFunctionWidgetPropertyDialog.h"
#include "vtkAxis.h"
#include "vtkChartXY.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRangedTransferFunctionDomain.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

class pqTransferFunctionWidgetPropertyWidget::pqInternals
{
public:
  pqInternals(pqTransferFunctionWidgetPropertyWidget* self) { this->Ui.setupUi(self); }

  Ui::TransferFunctionWidgetPropertyWidget Ui;
};

pqTransferFunctionWidgetPropertyWidget::pqTransferFunctionWidgetPropertyWidget(
  vtkSMProxy* smProxy, vtkSMProperty* property, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
  , Connection(nullptr)
  , Dialog(nullptr)
  , Internals(new pqTransferFunctionWidgetPropertyWidget::pqInternals(this))
{
  this->setProperty(property);
  vtkSMProxyProperty* proxyProperty = vtkSMProxyProperty::SafeDownCast(property);
  if (!proxyProperty)
  {
    qDebug() << "error, property is not a proxy property";
    return;
  }
  if (proxyProperty->GetNumberOfProxies() < 1)
  {
    // To uncomment once #17658 is fixed
    //    qDebug() << "error, no proxies for property";
    return;
  }

  vtkSMProxy* pxy = proxyProperty->GetProxy(0);
  if (!pxy)
  {
    qDebug() << "error, no proxy property";
    return;
  }
  this->TFProxy = vtkSMTransferFunctionProxy::SafeDownCast(pxy);

  this->Connection = vtkEventQtSlotConnect::New();
  this->Domain = proxyProperty->FindDomain<vtkSMRangedTransferFunctionDomain>();
  if (this->Domain)
  {
    this->Connection->Connect(
      this->Domain, vtkCommand::DomainModifiedEvent, this, SIGNAL(domainChanged()));
  }

  QAction* resetActn = new QAction(this->Internals->Ui.resetButton);
  resetActn->setToolTip("Reset using current data values");
  resetActn->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
  this->Internals->Ui.resetButton->addAction(resetActn);
  this->Internals->Ui.resetButton->setDefaultAction(resetActn);

  QObject::connect(this->Internals->Ui.editButton, &QPushButton::clicked, this,
    &pqTransferFunctionWidgetPropertyWidget::editButtonClicked);
  QObject::connect(
    this, SIGNAL(domainChanged()), this->Internals->Ui.resetButton, SLOT(highlight()));
  QObject::connect(this->Internals->Ui.resetButton, SIGNAL(clicked()),
    this->Internals->Ui.resetButton, SLOT(clear()));
  QObject::connect(resetActn, SIGNAL(triggered(bool)), this, SLOT(resetRangeToDomainDefault()));
  QObject::connect(resetActn, SIGNAL(triggered(bool)), this, SLOT(updateRange()));

  QObject::connect(this->Internals->Ui.rangeMin, &pqDoubleLineEdit::textChangedAndEditingFinished,
    this, &pqTransferFunctionWidgetPropertyWidget::onRangeEdited);
  QObject::connect(this->Internals->Ui.rangeMax, &pqDoubleLineEdit::textChangedAndEditingFinished,
    this, &pqTransferFunctionWidgetPropertyWidget::onRangeEdited);

  this->updateRange();
}

// -----------------------------------------------------------------------------
pqTransferFunctionWidgetPropertyWidget::~pqTransferFunctionWidgetPropertyWidget()
{
  if (this->Connection)
  {
    this->Connection->Delete();
  }
  delete this->Dialog;
}

// -----------------------------------------------------------------------------
void pqTransferFunctionWidgetPropertyWidget::resetRangeToDomainDefault()
{
  this->property()->ResetToDomainDefaults();
  Q_EMIT this->changeAvailable();
  Q_EMIT this->changeFinished();
}

// -----------------------------------------------------------------------------
void pqTransferFunctionWidgetPropertyWidget::propagateProxyPointsProperty()
{
  vtkSMProxy* pxy = static_cast<vtkSMProxy*>(this->TFProxy);
  vtkObjectBase* object = pxy->GetClientSideObject();
  vtkPiecewiseFunction* transferFunction = vtkPiecewiseFunction::SafeDownCast(object);

  int numPoints = transferFunction->GetSize();
  std::vector<double> functionPoints(numPoints * 4);
  double* pts = &functionPoints[0];

  for (int i = 0; i < numPoints; ++i)
  {
    int idx = i * 4;
    transferFunction->GetNodeValue(i, pts + idx);
  }

  vtkSMPropertyHelper(pxy, "Points").Set(pts, numPoints * 4);
  this->TFProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  Q_EMIT this->changeFinished();
}

// -----------------------------------------------------------------------------
void pqTransferFunctionWidgetPropertyWidget::editButtonClicked()
{
  delete this->Dialog;

  vtkObjectBase* object = this->TFProxy->GetClientSideObject();
  vtkPiecewiseFunction* transferFunction = vtkPiecewiseFunction::SafeDownCast(object);

  this->Dialog = new pqTransferFunctionWidgetPropertyDialog(
    this->property()->GetXMLLabel(), transferFunction, this, pqCoreUtilities::mainWidget());
  this->Dialog->setObjectName(this->property()->GetXMLName());
  this->Dialog->show();
}

// -----------------------------------------------------------------------------
void pqTransferFunctionWidgetPropertyWidget::updateRange()
{
  double range[2];
  this->TFProxy->GetRange(range);
  this->Internals->Ui.rangeMin->setText(QString::number(range[0]));
  this->Internals->Ui.rangeMax->setText(QString::number(range[1]));
}

// -----------------------------------------------------------------------------
void pqTransferFunctionWidgetPropertyWidget::onRangeEdited()
{
  this->TFProxy->RescaleTransferFunction(this->Internals->Ui.rangeMin->text().toDouble(),
    this->Internals->Ui.rangeMax->text().toDouble());
  Q_EMIT this->changeAvailable();
  Q_EMIT this->changeFinished();
}
