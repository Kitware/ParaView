// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqChooseColorPresetReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqPresetDialog.h"
#include "pqUndoStack.h"

#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionProxy.h"

#include "vtk_jsoncpp.h"

#include <QString>

#include <algorithm>
#include <cassert>
#include <vector>

QPointer<pqPresetDialog> pqChooseColorPresetReaction::PresetDialog;

//-----------------------------------------------------------------------------
pqChooseColorPresetReaction::pqChooseColorPresetReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , AllowsRegexpMatching(false)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqChooseColorPresetReaction::setActiveRepresentation);
    this->setActiveRepresentation();
  }
}

//-----------------------------------------------------------------------------
pqChooseColorPresetReaction::~pqChooseColorPresetReaction() = default;

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation == repr &&
    this->ColorMapEditorHelper->GetSelectedPropertiesType() == selectedPropertiesType)
  {
    return;
  }
  if (this->Representation)
  {
    this->disconnect(this->Representation);
    this->Representation = nullptr;
  }
  this->ColorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  if (repr)
  {
    this->Representation = repr;
    if (this->ColorMapEditorHelper->GetSelectedPropertiesType() ==
      vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks)
    {
      QObject::connect(repr, &pqDataRepresentation::blockColorTransferFunctionModified, this,
        &pqChooseColorPresetReaction::updateTransferFunction);
      QObject::connect(repr, &pqDataRepresentation::blockColorArrayNameModified, this,
        &pqChooseColorPresetReaction::updateTransferFunction);
    }
    else
    {
      QObject::connect(repr, &pqDataRepresentation::colorTransferFunctionModified, this,
        &pqChooseColorPresetReaction::updateTransferFunction);
      QObject::connect(repr, &pqDataRepresentation::colorArrayNameModified, this,
        &pqChooseColorPresetReaction::updateTransferFunction);
    }
  }
  this->updateTransferFunction();
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::updateTransferFunction()
{
  this->setTransferFunctions(
    this->Representation && this->Representation->getProxy()->GetProperty("LookupTable")
      ? this->ColorMapEditorHelper->GetSelectedLookupTables(this->Representation->getProxy())
      : std::vector<vtkSMProxy*>{});
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::setTransferFunctions(std::vector<vtkSMProxy*> luts)
{
  if (luts ==
    std::vector<vtkSMProxy*>(
      this->TransferFunctionProxies.begin(), this->TransferFunctionProxies.end()))
  {
    return;
  }
  this->TransferFunctionProxies.clear();
  for (auto& lut : luts)
  {
    this->TransferFunctionProxies.push_back(lut);
  }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::updateEnableState()
{
  const bool enabled = std::all_of(this->TransferFunctionProxies.begin(),
    this->TransferFunctionProxies.end(), [](vtkSMProxy* lut) { return lut != nullptr; });
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::onTriggered()
{
  this->choosePreset();
}

//-----------------------------------------------------------------------------
bool pqChooseColorPresetReaction::choosePreset(const char* presetName)
{
  if (this->TransferFunctionProxies.empty())
  {
    return false;
  }
  for (auto& lut : this->TransferFunctionProxies)
  {
    if (!lut)
    {
      return false;
    }
  }
  std::vector<bool> indexedLookups(this->TransferFunctionProxies.size(), false);
  for (size_t i = 0; i < this->TransferFunctionProxies.size(); ++i)
  {
    indexedLookups[i] =
      vtkSMPropertyHelper(this->TransferFunctionProxies[i], "IndexedLookup", true).GetAsInt() != 0;
  }
  const bool indexedLookup = *std::min_element(indexedLookups.begin(), indexedLookups.end());
  const bool maxIndexLookup = *std::max_element(indexedLookups.begin(), indexedLookups.end());
  if (indexedLookup != maxIndexLookup)
  {
    qWarning("Cannot apply presets to a mix of indexed and non-indexed lookup tables.");
    return false;
  }
  if (this->PresetDialog)
  {
    this->PresetDialog->setMode(indexedLookup ? pqPresetDialog::SHOW_INDEXED_COLORS_ONLY
                                              : pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  }
  else
  {
    // This should be deleted when the mainWidget is closed and it should be impossible
    // to get back here with the preset dialog open due to the event filtering done by
    // the preset dialog.
    this->PresetDialog = new pqPresetDialog(pqCoreUtilities::mainWidget(),
      indexedLookup ? pqPresetDialog::SHOW_INDEXED_COLORS_ONLY
                    : pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  }

  this->PresetDialog->setCurrentPreset(presetName);
  this->PresetDialog->setCustomizableLoadColors(!indexedLookup);
  this->PresetDialog->setCustomizableLoadOpacities(!indexedLookup);
  this->PresetDialog->setCustomizableUsePresetRange(!indexedLookup);
  this->PresetDialog->setCustomizableLoadAnnotations(indexedLookup);
  this->PresetDialog->setCustomizableAnnotationsRegexp(indexedLookup && this->AllowsRegexpMatching);
  this->connect(this->PresetDialog.data(), &pqPresetDialog::applyPreset, this,
    &pqChooseColorPresetReaction::applyCurrentPreset);
  this->PresetDialog->show();
  this->PresetDialog->raise();
  this->PresetDialog->activateWindow();
  return true;
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::applyCurrentPreset()
{
  pqPresetDialog* dialog = qobject_cast<pqPresetDialog*>(this->sender());
  assert(dialog);
  assert(dialog == this->PresetDialog);

  if (this->TransferFunctionProxies.empty())
  {
    return;
  }
  for (auto& lut : this->TransferFunctionProxies)
  {
    if (!lut)
    {
      return;
    }
  }

  BEGIN_UNDO_SET(tr("Apply color preset"));
  if (dialog->loadColors() || dialog->loadOpacities())
  {
    for (auto& lut : this->TransferFunctionProxies)
    {
      vtkSMProxy* sof = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy();
      if (dialog->loadColors())
      {
        vtkSMTransferFunctionProxy::ApplyPreset(
          lut, dialog->currentPreset(), !dialog->usePresetRange());
      }
      if (dialog->loadOpacities())
      {
        if (sof)
        {
          vtkSMTransferFunctionProxy::ApplyPreset(
            sof, dialog->currentPreset(), !dialog->usePresetRange());
        }
        else
        {
          qWarning("Cannot load opacities since 'ScalarOpacityFunction' is not present.");
        }
      }

      // We need to take extra care to avoid the color and opacity function ranges
      // from straying away from each other. This can happen if only one of them is
      // getting a preset and we're using the preset range.
      if (dialog->usePresetRange() && (dialog->loadColors() ^ dialog->loadOpacities()) && sof)
      {
        double range[2];
        if (dialog->loadColors() && vtkSMTransferFunctionProxy::GetRange(lut, range))
        {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(sof, range);
        }
        else if (dialog->loadOpacities() && vtkSMTransferFunctionProxy::GetRange(sof, range))
        {
          vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range);
        }
      }
    }
  }

  // When using Regexp or Annotation, Apply the preset annotation
  // on the Lookup table
  if (dialog->loadAnnotations() || dialog->regularExpression().isValid())
  {
    for (auto& lut : this->TransferFunctionProxies)
    {
      vtkSMTransferFunctionProxy::ApplyPreset(lut, dialog->currentPreset(), false);
    }
  }
  END_UNDO_SET();

  Q_EMIT this->presetApplied(
    QString(dialog->currentPreset().get("Name", "Preset").asString().c_str()));
}

//-----------------------------------------------------------------------------
QRegularExpression pqChooseColorPresetReaction::regularExpression()
{
  return this->PresetDialog ? this->PresetDialog->regularExpression() : QRegularExpression();
}

//-----------------------------------------------------------------------------
bool pqChooseColorPresetReaction::loadAnnotations()
{
  return this->PresetDialog ? this->PresetDialog->loadAnnotations() : false;
}
