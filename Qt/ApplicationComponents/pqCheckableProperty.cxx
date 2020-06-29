/*=========================================================================

   Program: ParaView
   Module:  pqCheckableProperty.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqCheckableProperty.h"

#include "pqPropertiesPanel.h"
#include "pqProxyWidget.h"
#include "pqView.h"
#include "vtkPVLogger.h"
#include "vtkSMPropertyGroup.h"

#include <QCheckBox>
#include <QComboBox>
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
  layoutLocal->setMargin(pqPropertiesPanel::suggestedMargin());
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
  checkBox->setToolTip(PropertyCheckBox->GetXMLLabel());
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
pqCheckableProperty::~pqCheckableProperty()
{
}

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
