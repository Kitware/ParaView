/*=========================================================================

   Program: ParaView
   Module:    pqCameraToolbar.cxx

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
#include "pqCameraToolbar.h"
#include "ui_pqCameraToolbar.h"

#include "pqActiveObjects.h"
#include "pqCameraReaction.h"
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
  new pqCameraReaction(ui.actionRotate90degCW, pqCameraReaction::ROTATE_CAMERA_CW);
  new pqCameraReaction(ui.actionRotate90degCCW, pqCameraReaction::ROTATE_CAMERA_CCW);

  new pqRenderViewSelectionReaction(
    ui.actionZoomToBox, nullptr, pqRenderViewSelectionReaction::ZOOM_TO_BOX);

  this->ZoomToDataAction = ui.actionZoomToData;
  this->ZoomToDataAction->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);

  this->ZoomClosestToDataAction = ui.actionZoomClosestToData;
  this->ZoomClosestToDataAction->setEnabled(pqActiveObjects::instance().activeSource() != nullptr);

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnabledState()));
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnabledState()));
}

//-----------------------------------------------------------------------------
void pqCameraToolbar::updateEnabledState()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  this->ZoomToDataAction->setEnabled(source && view);
  this->ZoomClosestToDataAction->setEnabled(source && view);
}
