/*=========================================================================

   Program: ParaView
   Module:    pqUse2DTransferFunctionReaction.cxx

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
#include "pqUse2DTransferFunctionReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

//-----------------------------------------------------------------------------
pqUse2DTransferFunctionReaction::pqUse2DTransferFunctionReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , TrackActiveObjects(track_active_objects)
{
  parentObject->setCheckable(true);
  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqDataRepresentation*)), this, SLOT(updateEnableState()),
    Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
pqUse2DTransferFunctionReaction::~pqUse2DTransferFunctionReaction() = default;

//-----------------------------------------------------------------------------
void pqUse2DTransferFunctionReaction::updateEnableState()
{
  pqDataRepresentation* cachedRepr = this->CachedRepresentation;
  this->setRepresentation(
    this->TrackActiveObjects ? pqActiveObjects::instance().activeRepresentation() : cachedRepr);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUse2DTransferFunctionReaction::representation() const
{
  return this->CachedRepresentation;
}

//-----------------------------------------------------------------------------
void pqUse2DTransferFunctionReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Links.clear();
  if (this->CachedRepresentation)
  {
    QObject::disconnect(this->CachedRepresentation, SIGNAL(colorArrayNameModified()), this,
      SLOT(updateEnableState()));
    QObject::disconnect(this->CachedRepresentation, SIGNAL(representationTypeModified()), this,
      SLOT(updateEnableState()));
    QObject::disconnect(this->CachedRepresentation, SIGNAL(useSeparateOpacityArrayModified()), this,
      SLOT(updateEnableState()));
  }
  this->CachedRepresentation = repr;
  if (repr)
  {
    // Observing `colorArrayNameModified` lets this reaction disable its parent action
    // when the representation is not using scalar coloring.
    // For example, when the color array combo box is set to "Solid Color".
    QObject::connect(repr, SIGNAL(colorArrayNameModified()), this, SLOT(updateEnableState()),
      Qt::QueuedConnection);
    QObject::connect(repr, SIGNAL(representationTypeModified()), this, SLOT(updateEnableState()),
      Qt::QueuedConnection);
    QObject::connect(repr, SIGNAL(useSeparateOpacityArrayModified()), this,
      SLOT(updateEnableState()), Qt::QueuedConnection);
  }

  // Recover proxy and action
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  QAction* action = this->parentAction();

  // Set action state
  vtkSMProperty* uoaProperty =
    reprProxy ? reprProxy->GetProperty("UseSeparateOpacityArray") : nullptr;

  // Do not allow usage of a 2D transfer function when the representation is already using
  // a separate array to map opacity to the scalar opacity function.
  bool usingSepOpacityArray =
    reprProxy && uoaProperty && vtkSMPropertyHelper(uoaProperty).GetAsInt() == 1;
  vtkSMProperty* useTf2DProperty = reprProxy ? reprProxy->GetProperty("UseTransfer2D") : nullptr;
  bool canUseTf2D = usingSepOpacityArray
    ? false
    : (reprProxy && useTf2DProperty && vtkSMRepresentationProxy::IsVolumeRendering(reprProxy) &&
        vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseTf2D);
  action->setChecked(false);
  if (useTf2DProperty && reprProxy)
  {
    this->Links.addPropertyLink(
      action, "checked", SIGNAL(toggled(bool)), reprProxy, useTf2DProperty);
  }
}
