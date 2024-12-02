// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPropArraySelectionWidget.h"

#include "pqArrayListDomain.h"
#include "pqArraySelectionWidget.h"
#include "pqComboBoxDomain.h"
#include "pqProxyWidget.h"
#include "pqSMAdaptor.h"
#include "pqSignalAdaptors.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropArrayListDomain.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMVectorProperty.h"
#include "vtkSmartPointer.h"

#include <QComboBox>
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
  QComboBox* ComboBox;

  QString PointArraysName;
  QString CellArraysName;
  QWidget* PointSelectorWidget;
  QWidget* CellSelectorWidget;

  vtkWeakPointer<vtkSMProxy> SMProxy;

  vtkWeakPointer<vtkSMStringVectorProperty> SMPointArrayProperty;
  vtkWeakPointer<vtkSMStringVectorProperty> SMCellArrayProperty;
  vtkWeakPointer<vtkSMStringVectorProperty> SelectedInputProperty;

  vtkWeakPointer<vtkSMDomain> SMPointDomain;
  vtkWeakPointer<vtkSMDomain> SMCellDomain;

  QMap<QString, QVariant> SavedPointLists;
  QMap<QString, QVariant> SavedCellLists;

  unsigned long PointObserverId;
  unsigned long CellObserverId;

  pqInternals()
    : PointObserverId(0)
  {
  }
  ~pqInternals()
  {
    if (this->SMPointDomain && this->PointObserverId)
    {
      this->SMPointDomain->RemoveObserver(this->PointObserverId);
    }
    this->PointObserverId = 0;

    if (this->SMCellDomain && this->CellObserverId)
    {
      this->SMCellDomain->RemoveObserver(this->CellObserverId);
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

  // Create combobox for prop selection
  QComboBox* combobox = new QComboBox(this);
  combobox->setObjectName("ComboBox");
  layout->addWidget(combobox);
  internals.ComboBox = combobox;

  auto propProp = smgroup->GetProperty("prop");
  vtkSMStringVectorProperty* selectedInputProperty =
    vtkSMStringVectorProperty::SafeDownCast(propProp);

  // Populate combobox with its domain values
  auto domain = vtkSMStringListDomain::SafeDownCast(propProp->GetDomain("prop"));
  int nbProps = domain->GetNumberOfStrings();

  combobox->setObjectName("ComboBox");
  combobox->setStyleSheet("combobox-popup: 0;");
  combobox->setMaxVisibleItems(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(propProp->GetHints()));

  pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(combobox);
  new pqComboBoxDomain(combobox, propProp);
  this->addPropertyLink(
    adaptor, "currentText", SIGNAL(currentTextChanged(QString)), selectedInputProperty);
  this->setChangeAvailableAsChangeFinished(true);

  // Create array selector widget for points
  auto propPointArrayProperty =
    vtkSMStringVectorProperty::SafeDownCast(smgroup->GetProperty("point_arrays"));
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

  this->Internals->PointSelectorWidget = pointArraySelectorWidget;
  this->Internals->CellSelectorWidget = cellArraySelectorWidget;
  this->Internals->PointArraysName = QString("point_arrays");
  this->Internals->CellArraysName = QString("cell_arrays");
  this->Internals->SMProxy = smproxy;
  this->Internals->SMPointArrayProperty = propPointArrayProperty;
  this->Internals->SMCellArrayProperty = propCellArrayProperty;
  this->Internals->SelectedInputProperty = selectedInputProperty;
  this->Internals->SMPointDomain = pointArrayListDomain;

  QObject::connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int i) {
    vtkWarningWithObjectMacro(nullptr,
      "prop changed to " << this->Internals->SelectedInputProperty->GetElement(0) << " index "
                         << i);

    QString currentProp = this->Internals->SelectedInputProperty->GetElement(0);
    this->Internals->SavedPointLists[currentProp] =
      this->Internals->PointSelectorWidget->property("point_arrays");
    this->Internals->SavedCellLists[currentProp] =
      this->Internals->CellSelectorWidget->property("cell_arrays");
  });

  this->Internals->PointObserverId = pointArrayListDomain->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &pqPropArraySelectionWidget::pointDomainChanged);
  this->Internals->CellObserverId = cellArrayListDomain->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &pqPropArraySelectionWidget::cellDomainChanged);

  this->addPropertyLink(
    pointArraySelectorWidget, "point_arrays", SIGNAL(widgetModified()), propPointArrayProperty);
  this->addPropertyLink(
    cellArraySelectorWidget, "cell_arrays", SIGNAL(widgetModified()), propCellArrayProperty);
  this->setChangeAvailableAsChangeFinished(true);

  // Change array selection when domain is updated
}

//-----------------------------------------------------------------------------
pqPropArraySelectionWidget::~pqPropArraySelectionWidget()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
bool pqPropArraySelectionWidget::event(QEvent* evt)
{
  auto& internals = (*this->Internals);
  if (evt->type() == QEvent::DynamicPropertyChange)
  {
    auto devt = dynamic_cast<QDynamicPropertyChangeEvent*>(evt);
    this->propertyChanged(devt->propertyName().data());
    return true;
  }

  return this->Superclass::event(evt);
}

void pqPropArraySelectionWidget::pointDomainChanged()
{
  // reset the widget's value using the domain.
  QVariant variantVal;
  vtkSMStringVectorProperty* prop =
    vtkSMStringVectorProperty::SafeDownCast(this->Internals->SMPointArrayProperty);

  vtkWarningWithObjectMacro(nullptr,
    "point domain changed, retrieving prop "
      << this->Internals->SelectedInputProperty->GetElement(0));

  this->Internals->SMProxy->UpdateProperty(this->Internals->SelectedInputProperty->GetXMLName());

  QString currentProp = this->Internals->SelectedInputProperty->GetElement(0);
  if (this->Internals->SavedPointLists.contains(currentProp))
  {
    this->Internals->PointSelectorWidget->setProperty(
      this->Internals->PointArraysName.toUtf8().data(),
      this->Internals->SavedPointLists[currentProp]);
    return;
  }

  vtkWarningWithObjectMacro(
    nullptr, "prop is " << this->Internals->SelectedInputProperty->GetElement(0));

  unsigned int nbPerCommand = prop->GetNumberOfElementsPerCommand();

  // if property has multiple string elements
  if (prop && nbPerCommand > 1 && prop->GetElementType(1) == vtkSMStringVectorProperty::STRING)
  {
    QList<QVariant> newPropList =
      pqSMAdaptor::getStringListProperty(this->Internals->SMPointArrayProperty);
    variantVal.setValue(newPropList);
  }
  else
  {
    QList<QList<QVariant>> newPropList = pqSMAdaptor::getSelectionProperty(
      this->Internals->SMPointArrayProperty, pqSMAdaptor::UNCHECKED);
    vtkWarningWithObjectMacro(nullptr, "new point prop list is size " << newPropList.size());

    variantVal.setValue(newPropList);
  }

  this->Internals->PointSelectorWidget->setProperty(
    this->Internals->PointArraysName.toUtf8().data(), variantVal);
}

void pqPropArraySelectionWidget::cellDomainChanged()
{
  // reset the widget's value using the domain.
  QVariant variantVal;
  vtkSMStringVectorProperty* prop = this->Internals->SMCellArrayProperty;

  vtkWarningWithObjectMacro(nullptr,
    "cell domain changed, retrieving prop "
      << this->Internals->SelectedInputProperty->GetElement(0));

  this->Internals->SMProxy->UpdateProperty(this->Internals->SelectedInputProperty->GetXMLName());

  QString currentProp = this->Internals->SelectedInputProperty->GetElement(0);
  if (this->Internals->SavedCellLists.contains(currentProp))
  {
    this->Internals->CellSelectorWidget->setProperty(
      this->Internals->CellArraysName.toUtf8().data(),
      this->Internals->SavedCellLists[currentProp]);
    return;
  }

  vtkWarningWithObjectMacro(
    nullptr, "prop is " << this->Internals->SelectedInputProperty->GetElement(0));

  unsigned int nbPerCommand = prop->GetNumberOfElementsPerCommand();

  // if property has multiple string elements
  if (prop && nbPerCommand > 1 && prop->GetElementType(1) == vtkSMStringVectorProperty::STRING)
  {
    QList<QVariant> newPropList =
      pqSMAdaptor::getStringListProperty(this->Internals->SMCellArrayProperty);
    variantVal.setValue(newPropList);
  }
  else
  {
    QList<QList<QVariant>> newPropList = pqSMAdaptor::getSelectionProperty(
      this->Internals->SMCellArrayProperty, pqSMAdaptor::UNCHECKED);
    vtkWarningWithObjectMacro(nullptr, "new cell prop list is size " << newPropList.size());

    variantVal.setValue(newPropList);
  }

  this->Internals->CellSelectorWidget->setProperty(
    this->Internals->CellArraysName.toUtf8().data(), variantVal);
}

//-----------------------------------------------------------------------------
void pqPropArraySelectionWidget::propertyChanged(const char* pname)
{
  auto& internals = (*this->Internals);

  const QVariant value = this->property(pname);
  if (!value.isValid() || !value.canConvert<QList<QVariant>>())
  {
    return;
  }

  auto listVariants = value.value<QList<QVariant>>();
  auto smprop = vtkSMVectorProperty::SafeDownCast(this->proxy()->GetProperty(pname));
  assert(smprop != nullptr);

  // if prop selected changed, change array selection too
}

//-----------------------------------------------------------------------------
void pqPropArraySelectionWidget::updateProperties()
{
  Q_EMIT this->widgetModified();
}
