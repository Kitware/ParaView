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
#include "vtkPVDataInformation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTransferFunction2DProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMTransferFunctionProxy.h"

namespace
{
vtkSMProxy* lutProxy(pqPipelineRepresentation* repr)
{
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy))
  {
    return vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();
  }
  return nullptr;
}
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeReaction::pqRescaleScalarRangeReaction(
  QAction* parentObject, bool track_active_objects, pqRescaleScalarRangeReaction::Modes mode)
  : Superclass(parentObject)
  , Mode(mode)
  , Connection(nullptr)
{
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(representationChanged(pqDataRepresentation*)), this,
      SLOT(setRepresentation(pqDataRepresentation*)));
    this->setRepresentation(pqActiveObjects::instance().activeRepresentation());

    if (this->Mode == TEMPORAL)
    {
      // Get ready to connect timekeepers with the reaction enabled state
      this->Connection = vtkSmartPointer<vtkEventQtSlotConnect>::New();
      pqServerManagerModel* model = pqApplicationCore::instance()->getServerManagerModel();
      this->connect(model, SIGNAL(serverAdded(pqServer*)), SLOT(onServerAdded(pqServer*)));
      this->connect(
        model, SIGNAL(aboutToRemoveServer(pqServer*)), SLOT(onAboutToRemoveServer(pqServer*)));
    }
  }
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeReaction::~pqRescaleScalarRangeReaction()
{
  if (this->Connection)
  {
    this->Connection->Disconnect();
  }
}

//-----------------------------------------------------------------------------
void pqRescaleScalarRangeReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Representation = qobject_cast<pqPipelineRepresentation*>(repr);
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
      pqRescaleScalarRangeReaction::rescaleScalarRangeToData(this->Representation);
      break;

    case CUSTOM:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(this->Representation);
      break;

    case TEMPORAL:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToDataOverTime(this->Representation);
      break;

    case VISIBLE:
      pqRescaleScalarRangeReaction::rescaleScalarRangeToVisible(this->Representation);
      break;
  }
}

//-----------------------------------------------------------------------------
bool pqRescaleScalarRangeReaction::rescaleScalarRangeToData(pqPipelineRepresentation* repr)
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

  BEGIN_UNDO_SET(tr("Reset transfer function ranges using data range"));
  repr->resetLookupTableScalarRange();
  repr->renderViewEventually();
  if (vtkSMProxy* lut = lutProxy(repr))
  {
    lut->UpdateVTKObjects();
  }
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog* pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
  pqPipelineRepresentation* repr)
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
    vtkSMPropertyHelper helper(proxy, "UseSeparateOpacityArray", true /*quiet*/);
    separateOpacity = helper.GetAsInt() == 1;
  }

  vtkSMProxy* lut = lutProxy(repr);
  pqRescaleScalarRangeToCustomDialog* dialog =
    pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(lut, separateOpacity);
  if (dialog != nullptr)
  {
    QObject::connect(
      dialog, &pqRescaleScalarRangeToCustomDialog::apply, [=]() { repr->renderViewEventually(); });
  }
  return dialog;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToCustomDialog* pqRescaleScalarRangeReaction::rescaleScalarRangeToCustom(
  vtkSMProxy* lut, bool separateOpacity)
{
  vtkSMTransferFunctionProxy* tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(lut);
  if (tfProxy == nullptr || lut == nullptr)
  {
    return nullptr;
  }

  double range[2] = { 0, 0 };
  if (!tfProxy->GetRange(range))
  {
    range[0] = 0;
    range[1] = 1.0;
  }

  int lockInfo = 0;
  vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode").Get(&lockInfo);

  pqRescaleScalarRangeToCustomDialog* dialog =
    new pqRescaleScalarRangeToCustomDialog(pqCoreUtilities::mainWidget());
  dialog->setLock(lockInfo == vtkSMTransferFunctionManager::NEVER);
  dialog->setRange(range[0], range[1]);
  dialog->showOpacityControls(separateOpacity);
  vtkSMTransferFunctionProxy* sofProxy = vtkSMTransferFunctionProxy::SafeDownCast(
    vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy());
  vtkSMTransferFunction2DProxy* tf2dProxy = vtkSMTransferFunction2DProxy::SafeDownCast(
    vtkSMPropertyHelper(lut, "TransferFunction2D", true).GetAsProxy());
  if (sofProxy && true)
  {
    if (!sofProxy->GetRange(range))
    {
      range[0] = 0;
      range[1] = 1.0;
    }
    dialog->setOpacityRange(range[0], range[1]);
  };
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();

  QObject::connect(dialog, &pqRescaleScalarRangeToCustomDialog::apply, [=]() {
    BEGIN_UNDO_SET(tr("Reset transfer function ranges"));
    double tRange[2];
    tRange[0] = dialog->minimum();
    tRange[1] = dialog->maximum();
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
        tf2dRange[1] = 0.0;
        tf2dRange[2] = 1.0;
      }
      tf2dRange[0] = tRange[0];
      tf2dRange[1] = tRange[1];
      tf2dProxy->RescaleTransferFunction(tf2dRange);
    }
    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    if (dialog->doLock())
    {
      vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode")
        .Set(vtkSMTransferFunctionManager::NEVER);
      lut->UpdateVTKObjects();
    }
    END_UNDO_SET();
  });

  return dialog;
}

//-----------------------------------------------------------------------------
pqRescaleScalarRangeToDataOverTimeDialog*
pqRescaleScalarRangeReaction::rescaleScalarRangeToDataOverTime(pqPipelineRepresentation* repr)
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

  int lockInfo = 0;
  if (vtkSMProxy* lut = lutProxy(repr))
  {
    vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode").Get(&lockInfo);
  }
  pqRescaleScalarRangeToDataOverTimeDialog* dialog =
    new pqRescaleScalarRangeToDataOverTimeDialog(pqCoreUtilities::mainWidget());
  dialog->setLock(lockInfo == vtkSMTransferFunctionManager::NEVER);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();

  QObject::connect(dialog, &pqRescaleScalarRangeToDataOverTimeDialog::apply, [=]() {
    BEGIN_UNDO_SET(tr("Reset transfer function ranges using temporal data range"));
    vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(repr->getProxy());

    // disable auto-rescale of transfer function since the user has set one
    // explicitly (BUG #14371).
    if (dialog->doLock())
    {
      if (vtkSMProxy* lut = lutProxy(repr))
      {
        vtkSMPropertyHelper(lut, "AutomaticRescaleRangeMode")
          .Set(vtkSMTransferFunctionManager::NEVER);
        lut->UpdateVTKObjects();
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

  BEGIN_UNDO_SET(tr("Reset transfer function ranges to visible data range"));
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
    this->Connection->Connect(timeKeeper->GetProperty("SuppressedTimeSources"),
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
    this->Connection->Disconnect(timeKeeper->GetProperty("SuppressedTimeSources"),
      vtkCommand::ModifiedEvent, this, SLOT(updateEnableState()));
  }
}
