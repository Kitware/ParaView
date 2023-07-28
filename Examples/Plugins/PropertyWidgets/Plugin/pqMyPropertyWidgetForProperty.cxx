// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMyPropertyWidgetForProperty.h"

#include "pqPropertiesPanel.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>

//-----------------------------------------------------------------------------
pqMyPropertyWidgetForProperty::pqMyPropertyWidgetForProperty(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->setShowLabel(false);

  QGridLayout* gridLayout = new QGridLayout(this);
  const int margin = pqPropertiesPanel::suggestedMargin();
  gridLayout->setContentsMargins(margin, margin, margin, margin);
  gridLayout->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  gridLayout->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  gridLayout->setColumnStretch(0, 0);
  gridLayout->setColumnStretch(1, 1);

  QLabel* customLabel = new QLabel(tr("Custom Widget"), this);
  gridLayout->addWidget(customLabel);

  QCheckBox* checkbox = new QCheckBox(tr("<-- pqMyPropertyWidgetForProperty"), this);
  checkbox->setObjectName("Checkbox");
  this->addPropertyLink(checkbox, "checked", SIGNAL(toggled(bool)), smproperty);
  gridLayout->addWidget(checkbox);

  // since there's no such thing a "editing" and 'editing done' for checkboxes.
  this->setChangeAvailableAsChangeFinished(true);
}

//-----------------------------------------------------------------------------
pqMyPropertyWidgetForProperty::~pqMyPropertyWidgetForProperty() = default;
