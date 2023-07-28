// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCheckableProperty.h"

#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkSMPropertyGroup.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>

struct pqCheckableProperty::Internal
{
  QPointer<pqPropertyWidget> propertyWidget;
  QPointer<QCheckBox> checkBox;
};

//-----------------------------------------------------------------------------
pqCheckableProperty::pqCheckableProperty(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , internal(new Internal)
{
  this->setShowLabel(true);
  vtkSMProperty* propertyName = smgroup->GetProperty("Property");
  if (!propertyName)
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(), "pqCheckableProperty: missing `Property` parameters");
    return;
  }

  vtkSMProperty* PropertyCheckBox = smgroup->GetProperty("PropertyCheckBox");
  if (!PropertyCheckBox)
  {
    vtkVLogF(PARAVIEW_LOG_PLUGIN_VERBOSITY(),
      "pqCheckableProperty: missing `PropertyCheckbox` parameters");
    return;
  }

  this->setChangeAvailableAsChangeFinished(true);

  auto* layoutLocal = new QHBoxLayout;
  layoutLocal->setContentsMargins(pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin(), pqPropertiesPanel::suggestedMargin(),
    pqPropertiesPanel::suggestedMargin());
  layoutLocal->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  this->setLayout(layoutLocal);

  // Main Property widget --------------------------------------
  auto widget = pqProxyWidget::createWidgetForProperty(propertyName, smproxy, this);

  if (!widget)
  {
    vtkVLogF(
      PARAVIEW_LOG_PLUGIN_VERBOSITY(), "pqCheckableProperty: `Property` with wrong parameters");
    return;
  }

  layoutLocal->addWidget(widget, /*stretch=*/1);

  // Trailing checkbox -----------------------------------------
  auto* checkBox = new QCheckBox(this);
  checkBox->setObjectName("CheckBox");
  checkBox->setToolTip(
    QCoreApplication::translate("ServerManagerXML", PropertyCheckBox->GetXMLLabel()));
  layoutLocal->addWidget(checkBox);

  this->addPropertyLink(checkBox, "checked", SIGNAL(toggled(bool)), PropertyCheckBox);
  this->connect(checkBox, SIGNAL(toggled(bool)), widget, SLOT(setEnabled(bool)));

  // Connect everything else -----------------------------------
  this->connect(widget, SIGNAL(viewChanged(pqView*)), SIGNAL(viewChanged(pqView*)));
  this->connect(widget, SIGNAL(changeAvailable()), SIGNAL(changeAvailable()));
  this->connect(widget, SIGNAL(changeFinished()), SIGNAL(changeFinished()));
  this->connect(widget, SIGNAL(restartRequired()), SIGNAL(restartRequired()));

  // Defaults, QCheckBox will not emit a signal unless there is a change.
  checkBox->setChecked(true);
  checkBox->setChecked(false);

  this->internal->checkBox = checkBox;
  this->internal->propertyWidget = widget;
}

//-----------------------------------------------------------------------------
pqCheckableProperty::~pqCheckableProperty() = default;

//-----------------------------------------------------------------------------
bool pqCheckableProperty::enableCheckbox() const
{
  if (this->internal->propertyWidget)
  {
    return this->internal->propertyWidget->isEnabled();
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::setEnableCheckbox(bool enableCheckbox)
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->setEnabled(enableCheckbox);
  }
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::apply()
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->apply();
  }

  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::reset()
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->reset();
  }

  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::select()
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->select();
  }

  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::deselect()
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->deselect();
  }

  this->Superclass::deselect();
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::updateWidget(bool showing_advanced_properties)
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->updateWidget(showing_advanced_properties);
  }

  this->Superclass::updateWidget(showing_advanced_properties);
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::setPanelVisibility(const char* vis)
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->setPanelVisibility(vis);
  }

  this->Superclass::setPanelVisibility(vis);
}

//-----------------------------------------------------------------------------
void pqCheckableProperty::setView(pqView* aview)
{
  if (this->internal->propertyWidget)
  {
    this->internal->propertyWidget->setView(aview);
  }

  this->Superclass::setView(aview);
}

//-----------------------------------------------------------------------------
bool pqCheckableProperty::isSingleRowItem() const
{
  return true;
}
