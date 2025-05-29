// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRescaleScalarRangeReaction.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRescaleScalarRangeToCustomDialog.h"
#include "pqRescaleScalarRangeToDataOverTimeDialog.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPVDataInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTransferFunction2DProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"

#include <algorithm>
#include <vector>

//-----------------------------------------------------------------------------
pqRescaleScalarRangeReaction::pqRescaleScalarRangeReaction(
  QAction* parentObject, bool track_active_objects, pqRescaleScalarRangeReaction::Modes mode)
  : Superclass(parentObject)
  , Mode(mode)
  , ServerAddedObserverId(0)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqRescaleScalarRangeReaction::setActiveRepresentation);
    this->setActiveRepresentation();

    if (this->Mode == TEMPORAL)
    {
      // Get ready to connect timekeepers with the reaction enabled state
      pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
      QObject::connect(model, &pqServerManagerModel::serverAdded, this,
        &pqRescaleScalarRangeReaction::onServerAdded);
      QObject::connect(model, &pqServerManagerModel::aboutToRemoveServer, this,
        &pqRescaleScalarRangeReaction::onAboutToRemoveServer);
    }
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeReaction::~pqRescaleScalarRangeReaction() = default;

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::setRepresentation(
  pqDataRepresentation* repr, int selectedPropertiesType)
{
  if (this->Representation != nullptr && this->Representation == repr &&
    this->SelectedPropertiesType == selectedPropertiesType)
  {
    return;
  }
  this->Representation = qobject_cast<pqPipelineRepresentation*>(repr);
  this->SelectedPropertiesType = selectedPropertiesType;
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::updateEnableState()
{
  bool enabled = this->Representation != nullptr;
  if (enabled && this->Mode == TEMPORAL)
  {
    pqPipelineSource* source = this->Representation->getInput();
    pqTimeKeeper* timeKeeper = source->getServer()->getTimeKeeper();
    enabled =
      (this->Representation->getOutputPortFromInput()->getDataInformation()->GetHasTime() != 0) &&
      vtkSMTimeKeeperProxy::IsTimeSourceTracked(timeKeeper->getProxy(), source->getProxy());
  }
  this->parentAction()->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::onTriggered()
{
  switch (this->Mode)
  {
    case DATA:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToData(
        this->Representation, this->SelectedPropertiesType);
      break;

    case CUSTOM:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
        this->Representation, this->SelectedPropertiesType);
      break;

    case TEMPORAL:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToDataOverTime(
        this->Representation, this->SelectedPropertiesType);
      break;

    case VISIBLE:
      // Visible does NOT support selectedPropertiesType == Blocks
      pqRescaleScalarRangeReaction::rescaleScalarRangeToVisible(this->Representation);
      break;
  }
}

//-----------------------------------------------------------------------------
bool pqRescaleScalarRangeReaction::rescaleScalarRangeToData(
  pqPipelineRepresentation* repr, int selectedPropertiesType)
{
  if (repr == nullptr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return false;
    }
  }

  const QString extraInfo =
    selectedPropertiesType == vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks
    ? tr("Block ")
    : QString();
  const QString undoText =
    tr("Reset ") + extraInfo + tr("Transfer Function Ranges Using Data Range");
  BEGIN_UNDO_SET(undoText);
  repr->resetLookupTableScalarRange(selectedPropertiesType);
  // no need to call lut->UpdateVTKObjects(), the resetLookupTableScalarRange call will do that
  repr->renderViewEventually();
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog* pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
  pqPipelineRepresentation* repr, int selectedPropertiesType)
{
  if (repr == nullptr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return nullptr;
    }
  }

  // See if we should show a separate opacity function range
  bool separateOpacity = false;
  auto proxy = repr->getProxy();
  if (proxy->GetProperty("UseSeparateOpacityArray"))
  {
    const vtkSMPropertyHelper helper(proxy, "UseSeparateOpacityArray", true /*quiet*/);
    separateOpacity = helper.GetAsInt() == 1;
  }

  vtkNew<vtkSMColorMapEditorHelper> colorMapEditorHelper;
  colorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  const std::vector<vtkSMProxy*> luts =
    colorMapEditorHelper->GetSelectedLookupTables(repr->getProxy());
  pqRescaleScalarRangeToCustomDialog* dialog =
    pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
      luts, separateOpacity, selectedPropertiesType);
  if (dialog != nullptr)
  {
    QObject::connect(
      dialog, &pqRescaleScalarRangeToCustomDialog::apply, [=]() { repr->renderViewEventually(); });
  }
  return dialog;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog* pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
  std::vector<vtkSMProxy*> luts, bool separateOpacity, int selectedPropertiesType)
{
  if (luts.empty())
  {
    return nullptr;
  }
  for (auto lut : luts)
  {
    auto tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(lut);
    if (!lut || !tfProxy)
    {
      return nullptr;
    }
  }

  double range[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  bool foundValidRange = false;
  for (auto lut : luts)
  {
    double lutRange[2];
    auto tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(lut);
    if (tfProxy->GetRange(lutRange))
    {
      range[0] = std::min(range[0], lutRange[0]);
      range[1] = std::max(range[1], lutRange[1]);
      foundValidRange = true;
    }
  }
  if (!foundValidRange)
  {
    range[0] = 0;
    range[1] = 1.0;
  }

  std::vector<int> locksInfo(luts.size(), 0);
  for (size_t i = 0; i < luts.size(); ++i)
  {
    vtkSMPropertyHelper(luts[i], "AutomaticRescaleRangeMode").Get(&locksInfo[i]);
  }
  const int lockInfo = *std::min_element(locksInfo.begin(), locksInfo.end());
  const int maxLockInfo = *std::max_element(locksInfo.begin(), locksInfo.end());
  if (maxLockInfo != lockInfo)
  {
    qWarning() << "Transfer functions have different lock states.";
    return nullptr;
  }

  pqRescaleScalarRangeToCustomDialog* dialog =
    new pqRescaleScalarRangeToCustomDialog(pqCoreUtilities::mainWidget());
  dialog->setLock(lockInfo == vtkSMTransferFunctionManager::NEVER);
  dialog->setRange(range[0], range[1]);
  dialog->showOpacityControls(separateOpacity);
  std::vector<vtkSMTransferFunctionProxy*> sofProxies;
  for (auto lut : luts)
  {
    auto sofProxy = vtkSMTransferFunctionProxy::SafeDownCast(
      vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy());
    sofProxies.push_back(sofProxy);
  }
  const bool hasAnySof = std::any_of(sofProxies.begin(), sofProxies.end(),
    [](vtkSMTransferFunctionProxy* sofProxy) { return sofProxy != nullptr; });
  std::vector<vtkSMTransferFunction2DProxy*> tf2dProxies;
  for (auto lut : luts)
  {
    auto tf2dProxy = vtkSMTransferFunction2DProxy::SafeDownCast(
      vtkSMPropertyHelper(lut, "TransferFunction2D", true).GetAsProxy());
    tf2dProxies.push_back(tf2dProxy);
  }
  if (hasAnySof)
  {
    foundValidRange = false;
    for (size_t i = 0; i < luts.size(); ++i)
    {
      if (sofProxies[i])
      {
        double sofRange[2];
        if (sofProxies[i]->GetRange(sofRange))
        {
          range[0] = std::min(range[0], sofRange[0]);
          range[1] = std::max(range[1], sofRange[1]);
          foundValidRange = true;
        }
      }
    }
    if (!foundValidRange)
    {
      range[0] = 0;
      range[1] = 1.0;
    }
    dialog->setOpacityRange(range[0], range[1]);
  }
  QObject::connect(dialog, &QWidget::close, dialog, &QObject::deleteLater);
  dialog->show();

  QObject::connect(dialog, &pqRescaleScalarRangeToCustomDialog::apply,
    [=]()
    {
      const QString extraInfo =
        selectedPropertiesType == vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks
        ? tr("Block ")
        : QString();
      const QString undoText =
        tr("Reset ") + extraInfo + tr("Transfer Function Ranges To Custom Range");
      BEGIN_UNDO_SET(undoText);
      double tRange[2];
      tRange[0] = dialog->minimum();
      tRange[1] = dialog->maximum();
      for (size_t i = 0; i < luts.size(); ++i)
      {
        auto tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(luts[i]);
        auto sofProxy = sofProxies[i];
        auto tf2dProxy = tf2dProxies[i];
        tfProxy->RescaleTransferFunction(tRange[0], tRange[1]);
        if (sofProxy)
        {
          // If we are using a separate opacity range, get those values from the GUI
          if (separateOpacity)
          {
            tRange[0] = dialog->opacityMinimum();
            tRange[1] = dialog->opacityMaximum();
          }
          vtkSMTransferFunctionProxy::RescaleTransferFunction(sofProxy, tRange[0], tRange[1]);
        }
        if (tf2dProxy)
        {
          double tf2dRange[4];
          if (!tf2dProxy->GetRange(tf2dRange))
          {
            tf2dRange[2] = 0.0;
            tf2dRange[3] = 1.0;
          }
          tf2dRange[0] = tRange[0];
          tf2dRange[1] = tRange[1];
          tf2dProxy->RescaleTransferFunction(tf2dRange);
        }
        // disable auto-rescale of transfer function since the user has set on
        // explicitly (BUG #14371).
        if (dialog->doLock())
        {
          for (auto lut : luts)
          {
            vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode")
              .Set(vtkSMTransferFunctionManager::NEVER);
            lut->UpdateVTKObjects();
          }
        }
      }
      END_UNDO_SET();
    });

  return dialog;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToDataOverTimeDialog*
pqRescaleScalarRangeReaction::rescaleScalarRangeToDataOverTime(
  pqPipelineRepresentation* repr, int selectedPropertiesType)
{
  if (repr == nullptr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return nullptr;
    }
  }

  vtkNew<vtkSMColorMapEditorHelper> colorMapEditorHelper;
  colorMapEditorHelper->SetSelectedPropertiesType(selectedPropertiesType);
  const std::vector<vtkSMProxy*> luts =
    colorMapEditorHelper->GetSelectedLookupTables(repr->getProxy());
  if (luts.empty())
  {
    return nullptr;
  }

  std::vector<int> locksInfo(luts.size(), 0);
  for (size_t i = 0; i < luts.size(); ++i)
  {
    vtkSMPropertyHelper(luts[i], "AutomaticRescaleRangeMode", /*quiet=*/true).Get(&locksInfo[i]);
  }
  const int lockInfo = *std::min_element(locksInfo.begin(), locksInfo.end());
  const int maxLockInfo = *std::max_element(locksInfo.begin(), locksInfo.end());
  if (maxLockInfo != lockInfo)
  {
    qWarning() << "Transfer functions have different lock states.";
    return nullptr;
  }
  pqRescaleScalarRangeToDataOverTimeDialog* dialog =
    new pqRescaleScalarRangeToDataOverTimeDialog(pqCoreUtilities::mainWidget());
  dialog->setLock(lockInfo == vtkSMTransferFunctionManager::NEVER);
  QObject::connect(dialog, &QWidget::close, dialog, &QObject::deleteLater);
  dialog->show();

  QObject::connect(dialog, &pqRescaleScalarRangeToDataOverTimeDialog::apply,
    [=]()
    {
      const QString extraInfo =
        selectedPropertiesType == vtkSMColorMapEditorHelper::SelectedPropertiesTypes::Blocks
        ? tr("Block ")
        : QString();
      const QString undoText =
        tr("Reset ") + extraInfo + tr("Transfer Function Ranges Using Temporal Data Range");
      BEGIN_UNDO_SET(undoText);
      repr->resetLookupTableScalarRangeOverTime(selectedPropertiesType);

      // disable auto-rescale of transfer function since the user has set one
      // explicitly (BUG #14371).
      if (dialog->doLock())
      {
        vtkNew<vtkSMColorMapEditorHelper> colorMapEditorHelper2;
        colorMapEditorHelper2->SetSelectedPropertiesType(selectedPropertiesType);
        if (colorMapEditorHelper2->GetAnySelectedUsingScalarColoring(repr->getProxy()))
        {
          for (auto& lut : colorMapEditorHelper2->GetSelectedLookupTables(repr->getProxy()))
          {
            if (lut)
            {
              vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode")
                .Set(vtkSMTransferFunctionManager::NEVER);
              lut->UpdateVTKObjects();
            }
          }
        }
      }
      repr->renderViewEventually();
      END_UNDO_SET();
    });
  return dialog;
}

//-----------------------------------------------------------------------------
bool pqRescaleScalarRangeReaction::rescaleScalarRangeToVisible(pqPipelineRepresentation* repr)
{
  if (repr == nullptr)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return false;
    }
  }

  pqView* view = repr->getView();
  if (!view)
  {
    qCritical() << "No view found.";
    return false;
  }

  BEGIN_UNDO_SET(tr("Reset Transfer Function Ranges To Visible Data Range"));
  vtkSMColorMapEditorHelper::RescaleTransferFunctionToVisibleRange(
    repr->getProxy(), view->getProxy());
  repr->renderViewEventually();
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::onServerAdded(pqServer* server)
{
  if (server)
  {
    // Connect new server timekeeper with the reaction enable state
    vtkSMProxy* timeKeeper = server->getTimeKeeper()->getProxy();
    this->ServerAddedObserverId =
      pqCoreUtilities::connect(timeKeeper->GetProperty("SuppressedTimeSources"),
        vtkCommand::ModifiedEvent, this, SLOT(updateEnableState()));
  }
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::onAboutToRemoveServer(pqServer* server)
{
  if (server)
  {
    // Disconnect previously connected timekeeper
    vtkSMProxy* timeKeeper = server->getTimeKeeper()->getProxy();
    if (this->ServerAddedObserverId != 0)
    {
      timeKeeper->GetProperty("SuppressedTimeSources")->RemoveObserver(this->ServerAddedObserverId);
      this->ServerAddedObserverId = 0;
    }
  }
}
