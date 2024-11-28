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
#include <QEvent>
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

  QString QProperty;
  QWidget* SelectorWidget;
  vtkWeakPointer<vtkSMProxy> SMProxy;
  vtkWeakPointer<vtkSMProperty> SMArrayProperty;
  vtkWeakPointer<vtkSMStringVectorProperty> SPropProperty;
  vtkWeakPointer<vtkSMDomain> SMDomain;
  QMap<QString, QVariant> SavedLists;
  bool changed = false;
  unsigned long ObserverId;

  pqInternals()
    : ObserverId(0)
  {
  }
  ~pqInternals()
  {
    if (this->SMDomain && this->ObserverId)
    {
      this->SMDomain->RemoveObserver(this->ObserverId);
    }
    this->ObserverId = 0;
  }
};

//-----------------------------------------------------------------------------
pqPropArraySelectionWidget::pqPropArraySelectionWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqPropArraySelectionWidget::pqInternals())
{
  this->setShowLabel(false);

  auto l = new QVBoxLayout(this);
  l->setSpacing(0);
  l->setContentsMargins(0, 0, 0, 0);

  auto& internals = (*this->Internals);

  // Create combobox for prop selection
  auto combobox = new QComboBox(this);
  combobox->setObjectName("ComboBox");
  l->addWidget(combobox);
  internals.ComboBox = combobox;

  auto propProp = smgroup->GetProperty("prop");
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(propProp);

  // Populate combobox with its domain values
  auto domain = vtkSMStringListDomain::SafeDownCast(propProp->GetDomain("prop"));
  int nbProps = domain->GetNumberOfStrings();

  combobox->setObjectName("ComboBox");
  combobox->setStyleSheet("combobox-popup: 0;");
  combobox->setMaxVisibleItems(
    pqPropertyWidget::hintsWidgetHeightNumberOfRows(propProp->GetHints()));

  pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(combobox);
  new pqComboBoxDomain(combobox, propProp);
  this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), svp);
  this->setChangeAvailableAsChangeFinished(true);

  // Create array selector widget, associate a domain widget with it
  auto propArrayProperty = smgroup->GetProperty("point_arrays");
  auto propArrayListDomain =
    vtkSMPropArrayListDomain::SafeDownCast(propArrayProperty->GetDomain("array_list"));
  this->PropArrayProperty = propArrayProperty;
  assert(propArrayListDomain != nullptr);
  QWidget* arraySelectorWidget = new pqArraySelectionWidget(this);
  l->addWidget(arraySelectorWidget);
  this->ArraySelectorWidget = arraySelectorWidget;

  this->Internals->SelectorWidget = arraySelectorWidget;
  this->Internals->QProperty = QString("point_arrays");
  this->Internals->SMProxy = smproxy;
  this->Internals->SMArrayProperty = propArrayProperty;
  this->Internals->SPropProperty = svp;
  this->Internals->SMDomain = propArrayListDomain;

  QObject::connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int i) {
    vtkWarningWithObjectMacro(nullptr,
      "prop changed to " << this->Internals->SPropProperty->GetElement(0) << " index " << i);
    QString currentProp = this->Internals->SPropProperty->GetElement(0);
    this->Internals->SavedLists[currentProp] =
      this->Internals->SelectorWidget->property("point_arrays");
  });

  // TODO: remove observer on destruction
  this->Internals->ObserverId = propArrayListDomain->AddObserver(
    vtkCommand::DomainModifiedEvent, this, &pqPropArraySelectionWidget::domainChanged);

  this->addPropertyLink(
    arraySelectorWidget, "point_arrays", SIGNAL(widgetModified()), propArrayProperty);
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

void pqPropArraySelectionWidget::domainChanged()
{
  // reset the widget's value using the domain.
  QVariant variantVal;
  vtkSMStringVectorProperty* prop =
    vtkSMStringVectorProperty::SafeDownCast(this->Internals->SMArrayProperty);

  vtkWarningWithObjectMacro(
    nullptr, "domain changed, retrieving prop " << this->Internals->SPropProperty->GetElement(0));

  this->Internals->SMProxy->UpdateProperty(this->Internals->SPropProperty->GetXMLName());

  QString currentProp = this->Internals->SPropProperty->GetElement(0);
  if (this->Internals->SavedLists.contains(currentProp))
  {
    this->Internals->SelectorWidget->setProperty(
      this->Internals->QProperty.toUtf8().data(), this->Internals->SavedLists[currentProp]);
    return;
  }

  vtkWarningWithObjectMacro(nullptr, "prop is " << this->Internals->SPropProperty->GetElement(0));

  unsigned int nbPerCommand = prop->GetNumberOfElementsPerCommand();

  // if property has multiple string elements
  if (prop && nbPerCommand > 1 && prop->GetElementType(1) == vtkSMStringVectorProperty::STRING)
  {
    QList<QVariant> newPropList =
      pqSMAdaptor::getStringListProperty(this->Internals->SMArrayProperty);
    variantVal.setValue(newPropList);
  }
  else
  {
    QList<QList<QVariant>> newPropList =
      pqSMAdaptor::getSelectionProperty(this->Internals->SMArrayProperty, pqSMAdaptor::UNCHECKED);
    vtkWarningWithObjectMacro(nullptr, "new prop list is size " << newPropList.size());

    variantVal.setValue(newPropList);
  }

  this->Internals->SelectorWidget->setProperty(
    this->Internals->QProperty.toUtf8().data(), variantVal);
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
  auto& internals = (*this->Internals);

  // U

  Q_EMIT this->widgetModified();
}
