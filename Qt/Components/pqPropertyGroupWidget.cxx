// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPropertyGroupWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>

#include "pqColorChooserButtonWithPalettes.h"
#include "pqComboBoxDomain.h"
#include "pqDoubleSliderWidget.h"
#include "pqSignalAdaptors.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqPropertyGroupWidget::pqPropertyGroupWidget(
  vtkSMProxy* _proxy, vtkSMPropertyGroup* smGroup, QWidget* _parent)
  : Superclass(_proxy, _parent)
  , PropertyGroup(smGroup)
{
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  pqColorChooserButton* color, const char* propertyName, int smindex)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
  {
    this->addPropertyLink(
      color, "chosenColorRgbF", SIGNAL(chosenColorChanged(const QColor&)), smProperty, smindex);
    if (pqColorChooserButtonWithPalettes* cbwp =
          qobject_cast<pqColorChooserButtonWithPalettes*>(color))
    {
      new pqColorPaletteLinkHelper(cbwp, this->proxy(), this->proxy()->GetPropertyName(smProperty));
    }
  }
  else
  {
    color->hide();
  }
}

void pqPropertyGroupWidget::addPropertyLink(QComboBox* cb, const char* propertyName, int smindex)
{
  vtkSMProperty* smproperty = this->PropertyGroup->GetProperty(propertyName);
  if (smproperty)
  {
    new pqComboBoxDomain(cb, smproperty);
    pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(cb);
    this->addPropertyLink(
      adaptor, "currentText", SIGNAL(currentTextChanged(QString)), smproperty, smindex);
  }
  else
  {
    cb->hide();
  }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QLineEdit* widget, const char* propertyName, int smindex)
{
  addStringPropertyLink(widget, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QCheckBox* button, const char* propertyName, int smindex)
{
  addCheckedPropertyLink(button, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QToolButton* button, const char* propertyName, int smindex)
{
  addCheckedPropertyLink(button, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QGroupBox* groupBox, const char* propertyName, int smindex)
{
  addCheckedPropertyLink(groupBox, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QDoubleSpinBox* spinBox, const char* propertyName, int smindex)
{
  addDoubleValuePropertyLink(spinBox, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QSpinBox* spinBox, const char* propertyName, int smindex)
{
  addIntValuePropertyLink(spinBox, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  pqDoubleSliderWidget* slider, const char* propertyName, int smindex)
{
  addDoubleValuePropertyLink(slider, propertyName, smindex);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addDoubleValuePropertyLink(
  QWidget* widget, const char* propertyName, int smindex)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
  {
    this->addPropertyLink(widget, "value", SIGNAL(valueChanged(double)), smProperty, smindex);
  }
  else
  {
    widget->hide();
  }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addIntValuePropertyLink(
  QWidget* widget, const char* propertyName, int smindex)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
  {
    this->addPropertyLink(widget, "value", SIGNAL(valueChanged(int)), smProperty, smindex);
  }
  else
  {
    widget->hide();
  }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addStringPropertyLink(
  QWidget* widget, const char* propertyName, int smindex)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
  {
    this->addPropertyLink(widget, "text", SIGNAL(editingFinished()), smProperty, smindex);
  }
  else
  {
    widget->hide();
  }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addCheckedPropertyLink(
  QWidget* widget, const char* propertyName, int smindex)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
  {
    this->addPropertyLink(widget, "checked", SIGNAL(toggled(bool)), smProperty, smindex);
  }
  else
  {
    widget->hide();
  }
}

//-----------------------------------------------------------------------------
char* pqPropertyGroupWidget::panelVisibility() const
{
  return this->PropertyGroup->GetPanelVisibility();
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::setPanelVisibility(const char* vis)
{
  return this->PropertyGroup->SetPanelVisibility(vis);
}
