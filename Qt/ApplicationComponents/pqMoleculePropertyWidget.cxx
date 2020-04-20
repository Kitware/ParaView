/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqMoleculePropertyWidget.h"
#include "ui_pqMoleculePropertyWidget.h"

#include "pqPropertiesPanel.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDocumentation.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMPVMoleculeRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QAction>

class pqMoleculePropertyWidget::pqInternals
{
public:
  Ui::MoleculePropertyWidget Ui;
  bool updatingFromPreset;
  bool showAdvancedProperties;

  pqInternals(pqMoleculePropertyWidget* self)
    : updatingFromPreset(false)
    , showAdvancedProperties(false)
  {
    this->Ui.setupUi(self);
    this->Ui.wdgLayout->setMargin(pqPropertiesPanel::suggestedMargin());
    this->Ui.wdgLayout->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
  }
};

//-----------------------------------------------------------------------------
pqMoleculePropertyWidget::pqMoleculePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(smproxy, smgroup, parentObject)
  , Internals(new pqInternals(this))
{
  Ui::MoleculePropertyWidget& ui = this->Internals->Ui;
  vtkSMProperty* prop = smgroup->GetProperty("AtomicRadiusFactor");
  auto domain = prop->FindDomain<vtkSMDoubleRangeDomain>();
  ui.atomicRadiusFactor->setMinimum(domain->GetMinimum(0));
  ui.atomicRadiusFactor->setMaximum(domain->GetMaximum(0));
  ui.atomicRadiusFactor->setResolution(
    domain->GetResolutionExists() ? domain->GetResolution() : 100);
  this->addPropertyLink(ui.atomicRadiusFactor, "AtomicRadiusFactor");
  this->setDocumentationAsTooltip(prop, ui.atomicRadiusFactor);
  QObject::connect(ui.atomicRadiusFactor, SIGNAL(valueEdited(double)), this, SLOT(resetPreset()));
  QObject::connect(ui.scaleAtomicRadiusFactor, &pqScaleByButton::scale, this,
    &pqMoleculePropertyWidget::onScaleAtomFactorChanged);
  QObject::connect(ui.resetAtomicRadiusFactor, &QToolButton::released, this,
    &pqMoleculePropertyWidget::onResetAtomFactorToggled);
  QAction* resetAtomicRadiusAction = new QAction(ui.resetAtomicRadiusFactor);
  resetAtomicRadiusAction->setToolTip("Reset the range values");
  resetAtomicRadiusAction->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
  ui.resetAtomicRadiusFactor->addAction(resetAtomicRadiusAction);
  ui.resetAtomicRadiusFactor->setDefaultAction(resetAtomicRadiusAction);

  this->addPropertyLink(ui.showAtoms, "RenderAtoms");
  QObject::connect(ui.showAtoms, &QCheckBox::toggled, this,
    &pqMoleculePropertyWidget::updateAtomWidgetsVisibility);
  this->setDocumentationAsTooltip(smgroup->GetProperty("RenderAtoms"), ui.showAtoms);

  prop = smgroup->GetProperty("AtomicRadiusType");
  auto enumDomain = prop->FindDomain<vtkSMEnumerationDomain>();
  for (unsigned int i = 0; i < enumDomain->GetNumberOfEntries(); i++)
  {
    ui.atomicRadiusType->addItem(enumDomain->GetEntryText(i));
  }
  this->addPropertyLink(ui.atomicRadiusType, "AtomicRadiusType");
  QObject::connect(ui.atomicRadiusType, &QComboBox::currentTextChanged, this,
    &pqMoleculePropertyWidget::updateAtomicRadiusWidgetsVisibility);
  this->setDocumentationAsTooltip(prop, ui.atomicRadiusType);

  prop = smgroup->GetProperty("AtomicRadiusArrayName");
  auto arrayDomain = prop->FindDomain<vtkSMArrayListDomain>();
  for (unsigned int i = 0; i < arrayDomain->GetNumberOfStrings(); i++)
  {
    ui.atomicRadiusArrayName->addItem(arrayDomain->GetString(i));
  }
  this->addPropertyLink(ui.atomicRadiusArrayName, "AtomicRadiusArrayName");
  this->setDocumentationAsTooltip(prop, ui.atomicRadiusArrayName);

  auto molproxy = vtkSMPVMoleculeRepresentationProxy::SafeDownCast(smproxy);
  for (int i = vtkSMPVMoleculeRepresentationProxy::Preset::None;
       i < vtkSMPVMoleculeRepresentationProxy::Preset::NbOfPresets; i++)
  {
    ui.preset->addItem(molproxy->GetPresetDisplayName(i));
  }
  QObject::connect(ui.preset, SIGNAL(currentIndexChanged(int)), this, SLOT(onPresetChanged(int)));
  ui.preset->setToolTip("Apply a preset to display properties, including advanced ones.");

  this->addPropertyLink(ui.showBonds, "RenderBonds");
  this->setDocumentationAsTooltip(smgroup->GetProperty("RenderBonds"), ui.showBonds);
  QObject::connect(ui.showBonds, &QCheckBox::toggled, this,
    &pqMoleculePropertyWidget::updateBondWidgetsVisibility);

  prop = smgroup->GetProperty("BondRadius");
  domain = prop->FindDomain<vtkSMDoubleRangeDomain>();
  ui.bondRadius->setMinimum(domain->GetMinimum(0));
  ui.bondRadius->setMaximum(domain->GetMaximum(0));
  ui.bondRadius->setResolution(domain->GetResolutionExists() ? domain->GetResolution() : 100);
  this->addPropertyLink(ui.bondRadius, "BondRadius");
  prop = smgroup->GetProperty("BondRadius");
  this->setDocumentationAsTooltip(prop, ui.bondRadius);
  QObject::connect(ui.bondRadius, SIGNAL(valueEdited(double)), this, SLOT(resetPreset()));
  QObject::connect(ui.scaleBondRadius, &pqScaleByButton::scale, this,
    &pqMoleculePropertyWidget::onScaleBondRadiusChanged);
  QObject::connect(ui.resetBondRadius, &QToolButton::released, this,
    &pqMoleculePropertyWidget::onResetBondRadiusToggled);
  QAction* resetBondRadiusAction = new QAction(ui.resetBondRadius);
  resetBondRadiusAction->setToolTip("Reset the range values");
  resetBondRadiusAction->setIcon(
    ui.resetBondRadius->style()->standardIcon(QStyle::SP_BrowserReload));
  ui.resetBondRadius->addAction(resetBondRadiusAction);
  ui.resetBondRadius->setDefaultAction(resetBondRadiusAction);

  this->addPropertyLink(ui.useMultiCylinders, "UseMultiCylindersForBonds");
  this->setDocumentationAsTooltip(
    smgroup->GetProperty("UseMultiCylindersForBonds"), ui.useMultiCylinders);
  QObject::connect(
    ui.useMultiCylinders, &QCheckBox::toggled, this, &pqMoleculePropertyWidget::resetPreset);

  this->addPropertyLink(ui.bondColorMode, "BondColorMode");
  this->setDocumentationAsTooltip(smgroup->GetProperty("BondColorMode"), ui.bondColorMode);
  QObject::connect(ui.bondColorMode, &QCheckBox::toggled, this,
    &pqMoleculePropertyWidget::updateBondColorWidgetVisibility);

  this->addPropertyLink(ui.bondColor, "BondColor");
  this->setDocumentationAsTooltip(smgroup->GetProperty("BondColor"), ui.bondColor);
  QObject::connect(ui.bondColor, &pqColorChooserButton::chosenColorChanged, this,
    &pqMoleculePropertyWidget::resetPreset);

  // Prevent first column to be resized when showing/hiding elements
  int minWidth = ui.atomicRadiusArrayNameLabel->width();
  for (int r = 0; r < ui.wdgLayout->rowCount(); r++)
  {
    auto item = ui.wdgLayout->itemAtPosition(r, 0);
    if (item && item->widget())
    {
      minWidth = std::max(minWidth, item->widget()->width());
    }
  }
  ui.wdgLayout->setColumnMinimumWidth(0, minWidth);

  this->updateBondWidgetsVisibility();
  this->updateAtomWidgetsVisibility();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::updateWidget(bool showing_advanced_properties)
{
  this->Internals->showAdvancedProperties = showing_advanced_properties;
  this->updateBondWidgetsVisibility();
  this->updateAtomWidgetsVisibility();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::onPresetChanged(int preset)
{
  this->Internals->updatingFromPreset = true;
  auto molProxy = vtkSMPVMoleculeRepresentationProxy::SafeDownCast(this->proxy());
  if (molProxy)
  {
    molProxy->SetPreset(preset);
    Q_EMIT this->changeAvailable();
  }

  this->Internals->updatingFromPreset = false;
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::updateBondWidgetsVisibility()
{
  Ui::MoleculePropertyWidget& ui = this->Internals->Ui;
  bool bondsEnabled = ui.showBonds->isChecked();
  bool advanced = this->Internals->showAdvancedProperties;
  ui.useMultiCylinders->setVisible(advanced && bondsEnabled);
  ui.multiCylindersLabel->setVisible(advanced && bondsEnabled);
  ui.bondRadius->setVisible(bondsEnabled);
  ui.bondRadiusLabel->setVisible(bondsEnabled);
  ui.bondColorMode->setVisible(advanced && bondsEnabled);
  ui.bondColorModeLabel->setVisible(advanced && bondsEnabled);
  ui.scaleBondRadius->setVisible(bondsEnabled);
  ui.resetBondRadius->setVisible(bondsEnabled);
  this->updateBondColorWidgetVisibility();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::updateBondColorWidgetVisibility()
{
  bool advanced = this->Internals->showAdvancedProperties;
  this->Internals->Ui.bondColor->setEnabled(!this->Internals->Ui.bondColorMode->isChecked());
  this->Internals->Ui.bondColor->setVisible(advanced && this->Internals->Ui.showBonds->isChecked());
  this->Internals->Ui.bondColorLabel->setVisible(
    advanced && this->Internals->Ui.showBonds->isChecked());
  this->resetPreset();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::updateAtomWidgetsVisibility()
{
  bool atomsEnabled = this->Internals->Ui.showAtoms->isChecked();
  bool advanced = this->Internals->showAdvancedProperties;
  this->Internals->Ui.atomicRadiusType->setVisible(advanced && atomsEnabled);
  this->Internals->Ui.atomicRadiusTypeLabel->setVisible(advanced && atomsEnabled);
  this->updateAtomicRadiusWidgetsVisibility();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::updateAtomicRadiusWidgetsVisibility()
{
  bool customArray =
    this->Internals->Ui.atomicRadiusType->currentText().compare("Input Array") == 0;
  bool atomsEnabled = this->Internals->Ui.showAtoms->isChecked();
  bool advanced = this->Internals->showAdvancedProperties;
  this->Internals->Ui.atomicRadiusFactorLabel->setVisible(atomsEnabled);
  this->Internals->Ui.atomicRadiusFactor->setVisible(atomsEnabled);
  this->Internals->Ui.atomicRadiusFactor->setEnabled(!customArray);
  this->Internals->Ui.atomicRadiusArrayNameLabel->setVisible(advanced && atomsEnabled);
  this->Internals->Ui.atomicRadiusArrayName->setVisible(advanced && atomsEnabled);
  this->Internals->Ui.atomicRadiusArrayName->setEnabled(customArray);
  this->Internals->Ui.scaleAtomicRadiusFactor->setVisible(atomsEnabled);
  this->Internals->Ui.scaleAtomicRadiusFactor->setEnabled(!customArray);
  this->Internals->Ui.resetAtomicRadiusFactor->setVisible(atomsEnabled);
  this->Internals->Ui.resetAtomicRadiusFactor->setEnabled(!customArray);

  this->resetPreset();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::resetPreset()
{
  if (!this->Internals->updatingFromPreset)
  {
    auto molProxy = vtkSMPVMoleculeRepresentationProxy::SafeDownCast(this->proxy());
    int index = molProxy ? molProxy->GetCurrentPreset() : 0;
    this->Internals->Ui.preset->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::setDocumentationAsTooltip(vtkSMProperty* prop, QWidget* widget)
{
  vtkSMDocumentation* doc = prop->GetDocumentation();

  if (doc != nullptr && widget != nullptr && doc->GetDescription())
  {
    widget->setToolTip(doc->GetDescription());
  }
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::onScaleAtomFactorChanged(double scale)
{
  this->Internals->Ui.atomicRadiusFactor->setValue(
    this->Internals->Ui.atomicRadiusFactor->value() * scale);
  this->resetPreset();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::onScaleBondRadiusChanged(double scale)
{
  this->Internals->Ui.bondRadius->setValue(this->Internals->Ui.bondRadius->value() * scale);
  this->resetPreset();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::onResetAtomFactorToggled()
{
  vtkSMProperty* prop = this->propertyGroup()->GetProperty("AtomicRadiusFactor");
  prop->ResetToDefault();
  this->proxy()->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->resetPreset();
}

//-----------------------------------------------------------------------------
void pqMoleculePropertyWidget::onResetBondRadiusToggled()
{
  vtkSMProperty* prop = this->propertyGroup()->GetProperty("BondRadius");
  prop->ResetToDefault();
  this->proxy()->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->resetPreset();
}
