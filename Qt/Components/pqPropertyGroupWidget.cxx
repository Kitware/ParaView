/*=========================================================================

   Program: ParaView
   Module:    pqPropertyGroupWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

========================================================================*/

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
