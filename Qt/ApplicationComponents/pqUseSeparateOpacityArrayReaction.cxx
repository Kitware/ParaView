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
        vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy));
  action->setEnabled(canUseSepOpacityArray);
  action->setChecked(false);
  if (useSepOpacityProp && reprProxy)
  {
    this->Links.addPropertyLink(
      action, "checked", SIGNAL(toggled(bool)), reprProxy, useSepOpacityProp);
  }
}
