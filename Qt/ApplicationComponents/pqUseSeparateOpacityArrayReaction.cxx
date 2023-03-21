/*=========================================================================

   Program: ParaView
   Module:    pqUseSeparateOpacityArrayReaction.cxx

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
#include "pqUseSeparateOpacityArrayReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"

//-----------------------------------------------------------------------------
pqUseSeparateOpacityArrayReaction::pqUseSeparateOpacityArrayReaction(
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
pqUseSeparateOpacityArrayReaction::~pqUseSeparateOpacityArrayReaction() = default;

//-----------------------------------------------------------------------------
void pqUseSeparateOpacityArrayReaction::updateEnableState()
{
  pqDataRepresentation* cachedRepr = this->CachedRepresentation;
  this->setRepresentation(
    this->TrackActiveObjects ? pqActiveObjects::instance().activeRepresentation() : cachedRepr);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUseSeparateOpacityArrayReaction::representation() const
{
  return this->CachedRepresentation;
}

//-----------------------------------------------------------------------------
void pqUseSeparateOpacityArrayReaction::setRepresentation(pqDataRepresentation* repr)
{
  this->Links.clear();
  if (this->CachedRepresentation)
  {
    QObject::disconnect(this->CachedRepresentation, SIGNAL(colorArrayNameModified()), this,
      SLOT(updateEnableState()));
    QObject::disconnect(this->CachedRepresentation, SIGNAL(representationTypeModified()), this,
      SLOT(updateEnableState()));
    QObject::disconnect(
      this->CachedRepresentation, SIGNAL(useTransfer2DModified()), this, SLOT(updateEnableState()));
  }
  this->CachedRepresentation = repr;
  if (repr)
  {
    QObject::connect(repr, SIGNAL(colorArrayNameModified()), this, SLOT(updateEnableState()),
      Qt::QueuedConnection);
    QObject::connect(repr, SIGNAL(representationTypeModified()), this, SLOT(updateEnableState()),
      Qt::QueuedConnection);
    QObject::connect(
      repr, SIGNAL(useTransfer2DModified()), this, SLOT(updateEnableState()), Qt::QueuedConnection);
  }

  // Recover proxy and action
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  QAction* action = this->parentAction();

  // Set action state
  vtkSMProperty* useSepOpacityProp =
    reprProxy ? reprProxy->GetProperty("UseSeparateOpacityArray") : nullptr;
  vtkSMProperty* useTf2DProperty = reprProxy ? reprProxy->GetProperty("UseTransfer2D") : nullptr;

  // Do not allow usage of a separate opacity array when the representation is
  // using a 2D transfer function.
  bool usingTf2D = useTf2DProperty ? vtkSMPropertyHelper(useTf2DProperty).GetAsInt() == 1 : false;
  bool canUseSepOpacityArray = usingTf2D
    ? false
    : (reprProxy && useSepOpacityProp && vtkSMRepresentationProxy::IsVolumeRendering(reprProxy) &&
        vtkSMPVRepresentationProxy::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseSepOpacityArray);
  action->setChecked(false);
  if (useSepOpacityProp && reprProxy)
  {
    this->Links.addPropertyLink(
      action, "checked", SIGNAL(toggled(bool)), reprProxy, useSepOpacityProp);
  }
}
