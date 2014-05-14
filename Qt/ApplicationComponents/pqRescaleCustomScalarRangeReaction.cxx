/*=========================================================================

   Program: ParaView
   Module:    pqRescaleCustomScalarRangeReaction.cxx

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
#include "pqRescaleCustomScalarRangeReaction.h"

#include "pqActiveObjects.h"
#include "pqPipelineRepresentation.h"
#include "pqRescaleRange.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMTransferFunctionProxy.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqRescaleCustomScalarRangeReaction::pqRescaleCustomScalarRangeReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)),
    this, SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqRescaleCustomScalarRangeReaction::updateEnableState()
{
  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(
    pqActiveObjects::instance().activeRepresentation());
  this->parentAction()->setEnabled(repr != NULL);
}

//-----------------------------------------------------------------------------
void pqRescaleCustomScalarRangeReaction::rescaleCustomScalarRange()
{
  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(
    pqActiveObjects::instance().activeRepresentation());
  if (!repr)
    {
    qCritical() << "No active representation.";
    return;
    }

  vtkSMProxy* rproxy = repr->getProxy();
  if (!rproxy || !vtkSMPVRepresentationProxy::GetUsingScalarColoring(rproxy))
    {
    return;
    }

  vtkSMProperty* lutProperty = rproxy->GetProperty("LookupTable");
  if (!lutProperty)
    {
    return;
    }

  vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty).GetAsProxy();
  vtkDiscretizableColorTransferFunction* stc =
    vtkDiscretizableColorTransferFunction::SafeDownCast(
      lut->GetClientSideObject());

  double range[2];
  stc->GetRange(range);

  pqRescaleRange dialog;
  dialog.setRange(range[0], range[1]);
  if (dialog.exec() == QDialog::Accepted)
    {
    range[0] = dialog.getMinimum();
    range[1] = dialog.getMaximum();

    BEGIN_UNDO_SET("Rescale Custom Range");
    vtkSMTransferFunctionProxy::RescaleTransferFunction(lut, range[0],range[1]);
    vtkSMTransferFunctionProxy::RescaleTransferFunction(
      vtkSMPropertyHelper(lut, "ScalarOpacityFunction").GetAsProxy(),
      range[0],range[1]);

    // disable auto-rescale of transfer function since the user has set on
    // explicitly.
    vtkSMProxy* lut = vtkSMPropertyHelper(lutProperty).GetAsProxy();
    vtkSMPropertyHelper(lut, "LockScalarRange", /*quiet*/ true).Set(1);
    lut->UpdateVTKObjects();
    repr->renderViewEventually();
    END_UNDO_SET();
    }
}
