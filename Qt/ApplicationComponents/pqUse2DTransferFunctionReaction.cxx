// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUse2DTransferFunctionReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

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
        vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseTf2D);
  action->setChecked(false);
  if (useTf2DProperty && reprProxy)
  {
    this->Links.addPropertyLink(
      action, "checked", SIGNAL(toggled(bool)), reprProxy, useTf2DProperty);
  }
}
