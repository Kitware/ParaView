// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraToolbar.h"
#include "ui_pqCameraToolbar.h"

#include "pqActiveObjects.h"
#include "pqCameraReaction.h"
#include "pqDataRepresentation.h"
#include "pqRenderViewSelectionReaction.h"

//-----------------------------------------------------------------------------
void pqCameraToolbar::constructor()
{
  Ui::pqCameraToolbar ui;
  ui.setupUi(this);
  new pqCameraReaction(ui.actionResetCamera, pqCameraReaction::RESET_CAMERA);
  new pqCameraReaction(ui.actionZoomToData, pqCameraReaction::ZOOM_TO_DATA);
  new pqCameraReaction(ui.actionResetCameraClosest, pqCameraReaction::RESET_CAMERA_CLOSEST);
  new pqCameraReaction(ui.actionZoomClosestToData, pqCameraReaction::ZOOM_CLOSEST_TO_DATA);
  new pqCameraReaction(ui.actionPositiveX, pqCameraReaction::RESET_POSITIVE_X);
  new pqCameraReaction(ui.actionNegativeX, pqCameraReaction::RESET_NEGATIVE_X);
  new pqCameraReaction(ui.actionPositiveY, pqCameraReaction::RESET_POSITIVE_Y);
  new pqCameraReaction(ui.actionNegativeY, pqCameraReaction::RESET_NEGATIVE_Y);
  new pqCameraReaction(ui.actionPositiveZ, pqCameraReaction::RESET_POSITIVE_Z);
  new pqCameraReaction(ui.actionNegativeZ, pqCameraReaction::RESET_NEGATIVE_Z);
  new pqCameraReaction(ui.actionIsometricView, pqCameraReaction::APPLY_ISOMETRIC_VIEW);
  new pqCameraReaction(ui.actionRotate90degCW, pqCameraReaction::ROTATE_CAMERA_CW);
  new pqCameraReaction(ui.actionRotate90degCCW, pqCameraReaction::ROTATE_CAMERA_CCW);

  new pqRenderViewSelectionReaction(
    ui.actionZoomToBox, nullptr, pqRenderViewSelectionReaction::ZOOM_TO_BOX);

  this->ZoomToDataAction = ui.actionZoomToData;
  this->ZoomToDataAction->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);

  this->ZoomClosestToDataAction = ui.actionZoomClosestToData;
  this->ZoomClosestToDataAction->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::sourceChanged, this,
    &pqCameraToolbar::onSourceChanged);
  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::viewChanged, this,
    &pqCameraToolbar::updateEnabledState);
  QObject::connect(&pqActiveObjects::instance(),
    QOverload<pqDataRepresentation*>::of(&pqActiveObjects::representationChanged), this,
    &pqCameraToolbar::onRepresentationChanged);
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::onSourceChanged(pqPipelineSource* source)
{
  if (this->SourceVisibilityChangedConnection)
  {
    // Make sure we have only one visibility changed qt connection
    QObject::disconnect(this->SourceVisibilityChangedConnection);
  }

  if (source)
  {
    this->SourceVisibilityChangedConnection = QObject::connect(
      source, &pqPipelineSource::visibilityChanged, this, &pqCameraToolbar::updateEnabledState);

    this->updateEnabledState();
  }
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::onRepresentationChanged(pqDataRepresentation* repr)
{
  if (this->RepresentationDataUpdatedConnection)
  {
    // Make sure we have only one data updated qt connection
    QObject::disconnect(this->RepresentationDataUpdatedConnection);
  }

  if (repr)
  {
    this->RepresentationDataUpdatedConnection = QObject::connect(
      repr, &pqDataRepresentation::dataUpdated, this, &pqCameraToolbar::updateEnabledState);

    this->updateEnabledState();
  }
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::updateEnabledState()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  this->ZoomToDataAction->setEnabled(source && view && repr && repr->isVisible());
  this->ZoomClosestToDataAction->setEnabled(source && view && repr && repr->isVisible());
}
