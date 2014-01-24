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
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QSpinBox>
#include "pqColorChooserButton.h"
#include "pqSignalAdaptors.h"
#include "pqStandardColorLinkAdaptor.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqPropertyGroupWidget::pqPropertyGroupWidget(
  vtkSMProxy *_proxy, vtkSMPropertyGroup* smGroup, QWidget *_parent):
  Superclass(_proxy, _parent),
  PropertyGroup (smGroup)
{
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  pqColorChooserButton* color, const char* propertyName)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
    {
    pqSignalAdaptorColor *adaptor =
      new pqSignalAdaptorColor(
        color, "chosenColor", SIGNAL(chosenColorChanged(const QColor&)), false);
    this->addPropertyLink(
      adaptor, "color", SIGNAL(colorChanged(const QVariant&)), smProperty);
    }
  else
    {
    color->hide();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QCheckBox* button, const char* propertyName)
{
  addCheckedPropertyLink (button, propertyName);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QGroupBox* groupBox, const char* propertyName)
{
  addCheckedPropertyLink (groupBox, propertyName);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QDoubleSpinBox* spinBox, const char* propertyName)
{
  addDoubleValuePropertyLink (spinBox, propertyName);
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addPropertyLink(
  QSpinBox* spinBox, const char* propertyName)
{
  addIntValuePropertyLink (spinBox, propertyName);
}


//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addDoubleValuePropertyLink(
  QWidget* widget, const char* propertyName)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
    {
    this->addPropertyLink(
      widget, "value", SIGNAL(valueChanged(double)), smProperty);
    }
  else
    {
    widget->hide();
    }
}

//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addIntValuePropertyLink(
  QWidget* widget, const char* propertyName)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
    {
    this->addPropertyLink(
      widget, "value", SIGNAL(valueChanged(int)), smProperty);
    }
  else
    {
    widget->hide();
    }
}


//-----------------------------------------------------------------------------
void pqPropertyGroupWidget::addCheckedPropertyLink(
  QWidget* widget, const char* propertyName)
{
  vtkSMProperty* smProperty = this->PropertyGroup->GetProperty(propertyName);
  if (smProperty)
    {
    this->addPropertyLink(
      widget, "checked", SIGNAL(toggled(bool)), smProperty);
    }
  else
    {
    widget->hide();
    }
}
