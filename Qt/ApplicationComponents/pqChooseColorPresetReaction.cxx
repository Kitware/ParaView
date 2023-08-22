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

#include <cassert>

QPointer<pqPresetDialog> pqChooseColorPresetReaction::PresetDialog;

namespace pvInternals
{
vtkSMProxy* lutProxy(vtkSMProxy* reprProxy)
{
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy))
  {
    return vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
pqChooseColorPresetReaction::pqChooseColorPresetReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , AllowsRegexpMatching(false)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(representationChanged(pqDataRepresentation*)), this,
      SLOT(setRepresentation(pqDataRepresentation*)));
    this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
  }
}

//-----------------------------------------------------------------------------
pqChooseColorPresetReaction::~pqChooseColorPresetReaction() = default;

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::setRepresentation(pqDataRepresentation* repr)
{
  if (this->Representation == repr)
  {
    return;
  }
  if (this->Representation)
  {
    this->disconnect(this->Representation);
  }
  this->Representation = repr;
  if (repr)
  {
    this->connect(repr, SIGNAL(colorTransferFunctionModified()), SLOT(updateTransferFunction()));
    this->connect(repr, SIGNAL(colorArrayNameModified()), SLOT(updateTransferFunction()));
  }
  this->updateTransferFunction();
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::updateTransferFunction()
{
  this->setTransferFunction(
    this->Representation ? this->Representation->getLookupTableProxy() : nullptr);
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::setTransferFunction(vtkSMProxy* lut)
{
  this->TransferFunctionProxy = lut;
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::updateEnableState()
{
  this->parentAction()->setEnabled(this->TransferFunctionProxy != nullptr);
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::onTriggered()
{
  this->choosePreset();
}

//-----------------------------------------------------------------------------
bool pqChooseColorPresetReaction::choosePreset(const char* presetName)
{
  vtkSMProxy* lut = this->TransferFunctionProxy;
  if (!lut)
  {
    return false;
  }

  bool indexedLookup = vtkSMPropertyHelper(lut, "IndexedLookup", true).GetAsInt() != 0;

  if (PresetDialog)
  {
    PresetDialog->setMode(indexedLookup ? pqPresetDialog::SHOW_INDEXED_COLORS_ONLY
                                        : pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  }
  else
  {
    // This should be deleted when the mainWidget is closed and it should be impossible
    // to get back here with the preset dialog open due to the event filtering done by
    // the preset dialog.
    PresetDialog = new pqPresetDialog(pqCoreUtilities::mainWidget(),
      indexedLookup ? pqPresetDialog::SHOW_INDEXED_COLORS_ONLY
                    : pqPresetDialog::SHOW_NON_INDEXED_COLORS_ONLY);
  }

  PresetDialog->setCurrentPreset(presetName);
  PresetDialog->setCustomizableLoadColors(!indexedLookup);
  PresetDialog->setCustomizableLoadOpacities(!indexedLookup);
  PresetDialog->setCustomizableUsePresetRange(!indexedLookup);
  PresetDialog->setCustomizableLoadAnnotations(indexedLookup);
  PresetDialog->setCustomizableAnnotationsRegexp(indexedLookup && this->AllowsRegexpMatching);
  this->connect(PresetDialog.data(), &pqPresetDialog::applyPreset, this,
    &pqChooseColorPresetReaction::applyCurrentPreset);
  PresetDialog->show();
  PresetDialog->raise();
  PresetDialog->activateWindow();
  return true;
}

//-----------------------------------------------------------------------------
void pqChooseColorPresetReaction::applyCurrentPreset()
{
  pqPresetDialog* dialog = qobject_cast<pqPresetDialog*>(this->sender());
  assert(dialog);
  assert(dialog == PresetDialog);

  vtkSMProxy* lut = this->TransferFunctionProxy;
  if (!lut)
  {
    return;
  }

  BEGIN_UNDO_SET(tr("Apply color preset"));
  if (dialog->loadColors() || dialog->loadOpacities())
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

  // When using Regexp or Annotation, Apply the preset annotation
  // on the Lookup table
  if (dialog->loadAnnotations() || dialog->regularExpression().isValid())
  {
    vtkSMTransferFunctionProxy::ApplyPreset(lut, dialog->currentPreset(), false);
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
