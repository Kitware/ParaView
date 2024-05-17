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
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqUse2DTransferFunctionReaction::setActiveRepresentation, Qt::QueuedConnection);
    this->setActiveRepresentation();
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqUse2DTransferFunctionReaction::~pqUse2DTransferFunctionReaction() = default;

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUse2DTransferFunctionReaction::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqUse2DTransferFunctionReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqUse2DTransferFunctionReaction::setRepresentation(pqDataRepresentation* repr)
{
  if (this->Representation != nullptr && this->Representation == repr)
  {
    return;
  }
  this->Links.clear();
  if (this->Representation)
  {
    this->disconnect(this->Representation);
    this->Representation = nullptr;
  }
  if (repr)
  {
    this->Representation = repr;
    // Observing `colorArrayNameModified` lets this reaction disable its parent action
    // when the representation is not using scalar coloring.
    // For example, when the color array combo box is set to "Solid Color".
    QObject::connect(repr, &pqDataRepresentation::colorArrayNameModified, this,
      &pqUse2DTransferFunctionReaction::updateEnableState, Qt::QueuedConnection);
    QObject::connect(repr, &pqDataRepresentation::representationTypeModified, this,
      &pqUse2DTransferFunctionReaction::updateEnableState, Qt::QueuedConnection);
    QObject::connect(repr, &pqDataRepresentation::useSeparateOpacityArrayModified, this,
      &pqUse2DTransferFunctionReaction::updateEnableState, Qt::QueuedConnection);
  }

  this->updateEnableState();

  // Recover proxy and action
  vtkSMProxy* reprProxy = repr ? repr->getProxy() : nullptr;
  QAction* action = this->parentAction();
  vtkSMProperty* useTf2DProperty = reprProxy ? reprProxy->GetProperty("UseTransfer2D") : nullptr;
  // add link between action and property
  if (useTf2DProperty && reprProxy)
  {
    this->Links.addPropertyLink(
      action, "checked", SIGNAL(toggled(bool)), reprProxy, useTf2DProperty);
  }
}

//-----------------------------------------------------------------------------
void pqUse2DTransferFunctionReaction::updateEnableState()
{
  // Recover proxy and action
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  QAction* action = this->parentAction();

  // Set action state
  vtkSMProperty* uoaProperty =
    reprProxy ? reprProxy->GetProperty("UseSeparateOpacityArray") : nullptr;

  // Do not allow usage of a 2D transfer function when the representation is already using
  // a separate array to map opacity to the scalar opacity function.
  const bool usingSepOpacityArray =
    reprProxy && uoaProperty && vtkSMPropertyHelper(uoaProperty).GetAsInt() == 1;
  vtkSMProperty* useTf2DProperty = reprProxy ? reprProxy->GetProperty("UseTransfer2D") : nullptr;
  const bool canUseTf2D = usingSepOpacityArray
    ? false
    : (reprProxy && useTf2DProperty && vtkSMRepresentationProxy::IsVolumeRendering(reprProxy) &&
        vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseTf2D);
  action->setChecked(false);
}
