/*=========================================================================

   Program: ParaView
   Module:    pqResetScalarRangeReaction.cxx

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
#include "pqResetScalarRangeReaction.h"
#include "ui_pqResetScalarRangeToDataOverTime.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRescaleRange.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"
#include "pqUndoStack.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QDebug>
#include <QSignalMapper>

namespace
{
vtkSMProxy* lutProxy(pqPipelineRepresentation* repr)
{
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : NULL;
  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy))
  {
    return vtkSMPropertyHelper(reprProxy, "LookupTable", true).GetAsProxy();
  }
  return NULL;
}
}

//-----------------------------------------------------------------------------
pqResetScalarRangeReaction::pqResetScalarRangeReaction(
  QAction* parentObject, bool track_active_objects, pqResetScalarRangeReaction::Modes mode)
  : Superclass(parentObject)
  , Mode(mode)
  , Connection(NULL)
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
pqResetScalarRangeReaction::~pqResetScalarRangeReaction()
{
  if (this->Connection)
  {
    this->Connection->Disconnect();
  }
}

//-----------------------------------------------------------------------------
void pqResetScalarRangeReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Representation = qobject_cast<pqPipelineRepresentation*>(repr);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqResetScalarRangeReaction::updateEnableState()
{
  bool enabled = this->Representation != NULL;
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
void pqResetScalarRangeReaction::onTriggered()
{
  switch (this->Mode)
  {
    case DATA:
      pqResetScalarRangeReaction::resetScalarRangeToData(this->Representation);
      break;

    case CUSTOM:
      pqResetScalarRangeReaction::resetScalarRangeToCustom(this->Representation);
      break;

    case TEMPORAL:
      pqResetScalarRangeReaction::resetScalarRangeToDataOverTime(this->Representation);
      break;

    case VISIBLE:
      pqResetScalarRangeReaction::resetScalarRangeToVisible(this->Representation);
      break;
  }
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToData(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return false;
    }
  }

  BEGIN_UNDO_SET("Reset transfer function ranges using data range");
  repr->resetLookupTableScalarRange();
  repr->renderViewEventually();
  if (vtkSMProxy* lut = lutProxy(repr))
  {
    // See BUG #17144. We unset LockScalarRange when resetting to data range.
    vtkSMPropertyHelper(lut, "LockScalarRange").Set(0);
    lut->UpdateVTKObjects();
  }
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToCustom(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return false;
    }
  }

  vtkSMProxy* lut = lutProxy(repr);
  if (pqResetScalarRangeReaction::resetScalarRangeToCustom(lut))
  {
    repr->renderViewEventually();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToCustom(vtkSMProxy* lut)
{
  vtkSMTransferFunctionProxy* tfProxy = vtkSMTransferFunctionProxy::SafeDownCast(lut);
  if (!tfProxy)
  {
    return false;
  }

  double range[2];
  if (!tfProxy->GetRange(range))
  {
    range[0] = 0;
    range[1] = 1.0;
  }

  pqRescaleRange dialog(pqCoreUtilities::mainWidget());
  dialog.setRange(range[0], range[1]);
  if (dialog.exec() == QDialog::Accepted)
  {
    BEGIN_UNDO_SET("Reset transfer function ranges");
    tfProxy->RescaleTransferFunction(dialog.minimum(), dialog.maximum());
    if (vtkSMProxy* sofProxy = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy())
    {
      vtkSMTransferFunctionProxy::RescaleTransferFunction(
        sofProxy, dialog.minimum(), dialog.maximum());
    }
    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    if (dialog.lock())
    {
      vtkSMPropertyHelper(lut, "LockScalarRange").Set(1);
      lut->UpdateVTKObjects();
    }
    END_UNDO_SET();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToDataOverTime(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
  {
    repr =
      qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
    if (!repr)
    {
      qCritical() << "No representation provided.";
      return false;
    }
  }

  QDialog dialog(pqCoreUtilities::mainWidget());
  Ui::ResetScalarRangeToDataOverTime ui;
  ui.setupUi(&dialog);

  QSignalMapper smapper;
  smapper.setMapping(ui.RescaleButton, QDialog::Accepted);
  smapper.connect(ui.RescaleButton, SIGNAL(clicked()), SLOT(map()));

  smapper.setMapping(ui.RescaleAndLockButton, static_cast<int>(QDialog::Accepted) + 1);
  smapper.connect(ui.RescaleAndLockButton, SIGNAL(clicked()), SLOT(map()));

  smapper.setMapping(ui.CancelButton, QDialog::Rejected);
  smapper.connect(ui.CancelButton, SIGNAL(clicked()), SLOT(map()));

  dialog.connect(&smapper, SIGNAL(mapped(int)), SLOT(done(int)));
  int retcode = dialog.exec();
  if (retcode != QDialog::Rejected)
  {
    BEGIN_UNDO_SET("Reset transfer function ranges using temporal data range");
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(repr->getProxy());

    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    if (retcode == static_cast<int>(QDialog::Accepted) + 1)
    {
      if (vtkSMProxy* lut = lutProxy(repr))
      {
        vtkSMPropertyHelper(lut, "LockScalarRange").Set(1);
        lut->UpdateVTKObjects();
      }
    }
    repr->renderViewEventually();
    END_UNDO_SET();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToVisible(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
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

  BEGIN_UNDO_SET("Reset transfer function ranges to visible data range");
  vtkSMPVRepresentationProxy::RescaleTransferFunctionToVisibleRange(
    repr->getProxy(), view->getProxy());
  repr->renderViewEventually();
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
void pqResetScalarRangeReaction::onServerAdded(pqServer* server)
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
void pqResetScalarRangeReaction::onAboutToRemoveServer(pqServer* server)
{
  if (server)
  {
    // Disconnect previously connected timekeeper
    vtkSMProxy* timeKeeper = server->getTimeKeeper()->getProxy();
    this->Connection->Disconnect(timeKeeper->GetProperty("SuppressedTimeSources"),
      vtkCommand::ModifiedEvent, this, SLOT(updateEnableState()));
  }
}
