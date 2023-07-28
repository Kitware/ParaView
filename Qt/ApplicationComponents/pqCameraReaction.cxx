// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraReaction.h"

#include "pqActiveObjects.h"
#include "pqPipelineRepresentation.h"
#include "pqRenderView.h"

#include "vtkCamera.h"
#include "vtkPVRenderViewSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"

//-----------------------------------------------------------------------------
pqCameraReaction::pqCameraReaction(QAction* parentObject, pqCameraReaction::Mode mode)
  : Superclass(parentObject)
{
  this->ReactionMode = mode;
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCameraReaction::updateEnableState()
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqPipelineSource* source = pqActiveObjects::instance().activeSource();
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  if (view && (this->ReactionMode == RESET_CAMERA || this->ReactionMode == RESET_CAMERA_CLOSEST))
  {
    this->parentAction()->setEnabled(true);
  }
  else if (rview)
  {
    if (this->ReactionMode == ZOOM_TO_DATA || this->ReactionMode == ZOOM_CLOSEST_TO_DATA)
    {
      this->parentAction()->setEnabled(source != nullptr);
    }
    else
    {
      // Check hints to see if actions should be disabled
      bool cameraResetButtonsEnabled = true;
      vtkPVXMLElement* hints = rview->getHints();
      if (hints)
      {
        cameraResetButtonsEnabled =
          hints->FindNestedElementByName("DisableCameraToolbarButtons") == nullptr;
      }

      this->parentAction()->setEnabled(cameraResetButtonsEnabled);
    }
  }
  else
  {
    this->parentAction()->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::onTriggered()
{
  switch (this->ReactionMode)
  {
    case RESET_CAMERA:
      this->resetCamera();
      break;

    case RESET_POSITIVE_X:
      this->resetPositiveX();
      break;

    case RESET_POSITIVE_Y:
      this->resetPositiveY();
      break;

    case RESET_POSITIVE_Z:
      this->resetPositiveZ();
      break;

    case RESET_NEGATIVE_X:
      this->resetNegativeX();
      break;

    case RESET_NEGATIVE_Y:
      this->resetNegativeY();
      break;

    case RESET_NEGATIVE_Z:
      this->resetNegativeZ();
      break;

    case APPLY_ISOMETRIC_VIEW:
      this->applyIsometricView();
      break;

    case ZOOM_TO_DATA:
      this->zoomToData();
      break;

    case ROTATE_CAMERA_CW:
      this->rotateCamera(-90.0);
      break;

    case ROTATE_CAMERA_CCW:
      this->rotateCamera(90.0);
      break;

    case ZOOM_CLOSEST_TO_DATA:
      this->zoomToData(true);
      break;

    case RESET_CAMERA_CLOSEST:
      this->resetCamera(true);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetCamera(bool closest)
{
  pqView* view = pqActiveObjects::instance().activeView();
  pqRenderView* ren = qobject_cast<pqRenderView*>(view);
  if (ren)
  {
    ren->resetCamera(closest, vtkPVRenderViewSettings::GetInstance()->GetZoomClosestOffsetRatio());
  }
  else if (view)
  {
    view->resetDisplay(closest);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetDirection(
  double look_x, double look_y, double look_z, double up_x, double up_y, double up_z)
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirection(look_x, look_y, look_z, up_x, up_y, up_z);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveX()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToPositiveX();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeX()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToNegativeX();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveY()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToPositiveY();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeY()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToNegativeY();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetPositiveZ()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToPositiveZ();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::resetNegativeZ()
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (ren)
  {
    ren->resetViewDirectionToNegativeZ();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::zoomToData(bool closest)
{
  pqRenderView* renModule = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  pqPipelineRepresentation* repr =
    qobject_cast<pqPipelineRepresentation*>(pqActiveObjects::instance().activeRepresentation());
  if (renModule && repr)
  {
    vtkSMRenderViewProxy* rm = renModule->getRenderViewProxy();
    rm->ZoomTo(repr->getProxy(), closest,
      vtkPVRenderViewSettings::GetInstance()->GetZoomClosestOffsetRatio());
    renModule->render();
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::rotateCamera(double angle)
{
  pqRenderView* renModule = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  if (renModule)
  {
    renModule->adjustRoll(angle);
  }
}

//-----------------------------------------------------------------------------
void pqCameraReaction::applyIsometricView()
{
  pqRenderView* renModule = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (renModule)
  {
    renModule->applyIsometricView();
  }
}
