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

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqPipelineRepresentation.h"
#include "pqRescaleRange.h"
#include "pqUndoStack.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QDebug>
#include <QMessageBox>

namespace
{
  vtkSMProxy* lutProxy(pqPipelineRepresentation* repr)
    {
    vtkSMProxy* reprProxy = repr? repr->getProxy() : NULL;
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
  : Superclass(parentObject),
  Mode(mode)
{
  if (track_active_objects)
    {
    QObject::connect(&pqActiveObjects::instance(),
      SIGNAL(representationChanged(pqDataRepresentation*)),
      this, SLOT(setRepresentation(pqDataRepresentation*)));
    this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
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
  this->parentAction()->setEnabled(this->Representation != NULL);
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
    }
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToData(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
    {
    repr = qobject_cast<pqPipelineRepresentation*>(
      pqActiveObjects::instance().activeRepresentation());
    if (!repr)
      {
      qCritical() << "No representation provided.";
      return false;
      }
    }

  BEGIN_UNDO_SET("Reset transfer function ranges using data range");
  repr->resetLookupTableScalarRange();
  repr->renderViewEventually();
  END_UNDO_SET();
  return true;
}

//-----------------------------------------------------------------------------
bool pqResetScalarRangeReaction::resetScalarRangeToCustom(pqPipelineRepresentation* repr)
{
  if (repr == NULL)
    {
    repr = qobject_cast<pqPipelineRepresentation*>(
      pqActiveObjects::instance().activeRepresentation());
    if (!repr)
      {
      qCritical() << "No representation provided.";
      return false;
      }
    }

  vtkSMProxy* lut = lutProxy(repr);
  if (!lut)
    {
    return false;
    }

  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(lut->GetClientSideObject());
  double range[2];
  stc->GetRange(range);

  pqRescaleRange dialog(pqCoreUtilities::mainWidget());
  dialog.setRange(range[0], range[1]);
  if (dialog.exec() == QDialog::Accepted)
    {
    BEGIN_UNDO_SET("Reset transfer function ranges");
    vtkSMTransferFunctionProxy::RescaleTransferFunction(
      lut, dialog.getMinimum(), dialog.getMaximum());
    if (vtkSMProxy* sofProxy = vtkSMPropertyHelper(lut, "ScalarOpacityFunction", true).GetAsProxy())
      {
      vtkSMTransferFunctionProxy::RescaleTransferFunction(
        sofProxy, dialog.getMinimum(), dialog.getMaximum());
      }
    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    vtkSMPropertyHelper(lut, "LockScalarRange").Set(1);
    lut->UpdateVTKObjects();
    repr->renderViewEventually();
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
    repr = qobject_cast<pqPipelineRepresentation*>(
      pqActiveObjects::instance().activeRepresentation());
    if (!repr)
      {
      qCritical() << "No representation provided.";
      return false;
      }
    }

  if (QMessageBox::warning(pqCoreUtilities::mainWidget(),
      "Potentially slow operation",
      "This can potentially take a long time to complete. \n"
      "Are you sure you want to continue?",
      QMessageBox::Yes |QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
    BEGIN_UNDO_SET("Reset transfer function ranges using temporal data range");
    vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRangeOverTime(repr->getProxy());

    // disable auto-rescale of transfer function since the user has set on
    // explicitly (BUG #14371).
    if (vtkSMProxy* lut = lutProxy(repr))
      {
      vtkSMPropertyHelper(lut, "LockScalarRange").Set(1);
      lut->UpdateVTKObjects();
      }
    repr->renderViewEventually();
    END_UNDO_SET();
    return true;
    }
  return false;
}
