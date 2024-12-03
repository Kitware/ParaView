// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPropArraySelectionWidget.h"

#include "pqArrayListDomain.h"
#include "pqArraySelectionWidget.h"
#include "pqProxyWidget.h"
#include "pqSMAdaptor.h"
#include "pqSignalAdaptors.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropArrayListDomain.h"
#include "vtkSMPropDomain.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QPointer>
#include <QScopedValueRollback>
#include <QVBoxLayout>
#include <QtDebug>

#include <cassert>
#include <map>
#include <string>
#include <utility>
#include <vector>

class pqPropArraySelectionWidget::pqInternals
{
public:
  QString PointArraysName;
  QString CellArraysName;
  QString PreviouslySelectedProp;
  QWidget* PointSelectorWidget;
  QWidget* CellSelectorWidget;

  vtkWeakPointer<vtkSMProxy> SMProxy;

  vtkWeakPointer<vtkSMStringVectorProperty> SMPointArrayProperty;
  vtkWeakPointer<vtkSMStringVectorProperty> SMCellArrayProperty;

  vtkWeakPointer<vtkSMPropArrayListDomain> PointArrayListDomain;
  vtkWeakPointer<vtkSMPropArrayListDomain> CellArrayListDomain;

  unsigned long PointObserverId;
  unsigned long CellObserverId;

  pqInternals()
    : PointObserverId(0)
  {
  }
  ~pqInternals()
  {
    if (this->PointArrayListDomain && this->PointObserverId)
    {
      this->PointArrayListDomain->RemoveObserver(this->PointObserverId);
    }
    this->PointObserverId = 0;

    if (this->CellArrayListDomain && this->CellObserverId)
    {
      this->CellArrayListDomain->RemoveObserver(this->CellObserverId);
    }
    this->CellObserverId = 0;
  }
};

//-----------------------------------------------------------------------------
pqPropArraySelectionWidget::pqPropArraySelectionWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqPropArraySelectionWidget::pqInternals())
{
  this->setShowLabel(false);

  auto layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  auto& internals = (*this->Internals);

  auto label = new QLabel(tr("Select arrays to export"), this);
  layout->addWidget(label);

  // Create array selector widget for points
  auto pointArrayListDomain =
    vtkSMPropArrayListDomain::SafeDownCast(propPointArrayProperty->GetDomain("array_list"));
  assert(pointArrayListDomain != nullptr);
  pqArraySelectionWidget* pointArraySelectorWidget = new pqArraySelectionWidget(this);
  pointArraySelectorWidget->setIconType("point_arrays", "point");
  pointArraySelectorWidget->setObjectName("PointSelectionWidget");
  pointArraySelectorWidget->setHeaderLabel(
    QCoreApplication::translate("ServerManagerXML", propPointArrayProperty->GetXMLLabel()));
  layout->addWidget(pointArraySelectorWidget);

  // Create array selector widget for cells
  auto propCellArrayProperty =
    vtkSMStringVectorProperty::SafeDownCast(smgroup->GetProperty("cell_arrays"));
  auto cellArrayListDomain =
    vtkSMPropArrayListDomain::SafeDownCast(propCellArrayProperty->GetDomain("array_list"));
  assert(cellArrayListDomain != nullptr);
  pqArraySelectionWidget* cellArraySelectorWidget = new pqArraySelectionWidget(this);
  cellArraySelectorWidget->setIconType("cell_arrays", "cell");
  cellArraySelectorWidget->setObjectName("CellSelectionWidget");
  cellArraySelectorWidget->setHeaderLabel(
    QCoreApplication::translate("ServerManagerXML", propCellArrayProperty->GetXMLLabel()));
  layout->addWidget(cellArraySelectorWidget);

  // Setup internal structures
  this->Internals->PointSelectorWidget = pointArraySelectorWidget;
  this->Internals->CellSelectorWidget = cellArraySelectorWidget;
  this->Internals->CellArrayListDomain = cellArrayListDomain;
  this->Internals->PointArrayListDomain = pointArrayListDomain;
  this->Internals->PointArraysName = QString("point_arrays");
  this->Internals->CellArraysName = QString("cell_arrays");
  this->Internals->SMProxy = smproxy;
  this->Internals->SMPointArrayProperty = propPointArrayProperty;
  this->Internals->SMCellArrayProperty = propCellArrayProperty;

  this->Internals->PointObserverId = pointArrayListDomain->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &pqPropArraySelectionWidget::pointDomainChanged);
  this->Internals->CellObserverId = cellArrayListDomain->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &pqPropArraySelectionWidget::cellDomainChanged);

  this->addPropertyLink(
    pointArraySelectorWidget, "point_arrays", SIGNAL(widgetModified()), propPointArrayProperty);
  this->addPropertyLink(
    cellArraySelectorWidget, "cell_arrays", SIGNAL(widgetModified()), propCellArrayProperty);
  this->setChangeAvailableAsChangeFinished(true);
}

//-----------------------------------------------------------------------------
pqPropArraySelectionWidget::~pqPropArraySelectionWidget()
{
  delete this->Internals;
}

void pqPropArraySelectionWidget::pointDomainChanged()
{
  QList<QList<QVariant>> newPropList = pqSMAdaptor::getSelectionProperty(
    this->Internals->SMPointArrayProperty, pqSMAdaptor::UNCHECKED);

  QVariant variantVal = QVariant::fromValue(newPropList);
  this->Internals->PointSelectorWidget->setProperty(
    this->Internals->PointArraysName.toUtf8().data(), variantVal);
}

//-----------------------------------------------------------------------------
void pqPropArraySelectionWidget::cellDomainChanged()
{
  QList<QList<QVariant>> newPropList =
    pqSMAdaptor::getSelectionProperty(this->Internals->SMCellArrayProperty, pqSMAdaptor::UNCHECKED);

  QVariant variantVal = QVariant::fromValue(newPropList);
  this->Internals->CellSelectorWidget->setProperty(
    this->Internals->CellArraysName.toUtf8().data(), variantVal);
}

//-----------------------------------------------------------------------------
void pqPropArraySelectionWidget::updateProperties()
{
  Q_EMIT this->widgetModified();
}
