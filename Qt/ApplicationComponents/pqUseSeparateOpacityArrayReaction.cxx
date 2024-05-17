// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUseSeparateOpacityArrayReaction.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRepresentationProxy.h"

//-----------------------------------------------------------------------------
pqUseSeparateOpacityArrayReaction::pqUseSeparateOpacityArrayReaction(
  QAction* parentObject, bool track_active_objects)
  : Superclass(parentObject)
  , TrackActiveObjects(track_active_objects)
{
  parentObject->setCheckable(true);
  if (track_active_objects)
  {
    QObject::connect(&pqActiveObjects::instance(),
      QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
      &pqUseSeparateOpacityArrayReaction::setActiveRepresentation, Qt::QueuedConnection);
    this->setActiveRepresentation();
  }
  else
  {
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
pqUseSeparateOpacityArrayReaction::~pqUseSeparateOpacityArrayReaction() = default;

//-----------------------------------------------------------------------------
pqDataRepresentation* pqUseSeparateOpacityArrayReaction::representation() const
{
  return this->Representation;
}

//-----------------------------------------------------------------------------
void pqUseSeparateOpacityArrayReaction::setActiveRepresentation()
{
  this->setRepresentation(pqActiveObjects::instance().activeRepresentation());
}

//-----------------------------------------------------------------------------
void pqUseSeparateOpacityArrayReaction::setRepresentation(pqDataRepresentation* repr)
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
    QObject::connect(repr, &pqDataRepresentation::colorArrayNameModified, this,
      &pqUseSeparateOpacityArrayReaction::updateEnableState, Qt::QueuedConnection);
    QObject::connect(repr, &pqDataRepresentation::representationTypeModified, this,
      &pqUseSeparateOpacityArrayReaction::updateEnableState, Qt::QueuedConnection);
    QObject::connect(repr, &pqDataRepresentation::useTransfer2DModified, this,
      &pqUseSeparateOpacityArrayReaction::updateEnableState, Qt::QueuedConnection);
  }

  this->updateEnableState();

  // Recover proxy and action
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  vtkSMProperty* useSepOpacityProp =
    reprProxy ? reprProxy->GetProperty("UseSeparateOpacityArray") : nullptr;
  // add link between action and property
  if (useSepOpacityProp && reprProxy)
  {
    this->Links.addPropertyLink(
      this->parentAction(), "checked", SIGNAL(toggled(bool)), reprProxy, useSepOpacityProp);
  }
}

//-----------------------------------------------------------------------------
void pqUseSeparateOpacityArrayReaction::updateEnableState()
{
  // Recover proxy and action
  vtkSMProxy* reprProxy = this->Representation ? this->Representation->getProxy() : nullptr;
  QAction* action = this->parentAction();
  // Set action state
  vtkSMProperty* useSepOpacityProp =
    reprProxy ? reprProxy->GetProperty("UseSeparateOpacityArray") : nullptr;
  vtkSMProperty* useTf2DProperty = reprProxy ? reprProxy->GetProperty("UseTransfer2D") : nullptr;

  // Do not allow usage of a separate opacity array when the representation is
  // using a 2D transfer function.
  const bool usingTf2D =
    useTf2DProperty ? vtkSMPropertyHelper(useTf2DProperty).GetAsInt() == 1 : false;
  const bool canUseSepOpacityArray = usingTf2D
    ? false
    : (reprProxy && useSepOpacityProp && vtkSMRepresentationProxy::IsVolumeRendering(reprProxy) &&
        vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseSepOpacityArray);
  action->setChecked(false);
}
