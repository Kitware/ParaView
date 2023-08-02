// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  resetActn->setToolTip(tr("Reset using current data values"));
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
    QCoreApplication::translate("ServerManagerXML", this->property()->GetXMLLabel()),
    transferFunction, this, pqCoreUtilities::mainWidget());
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
