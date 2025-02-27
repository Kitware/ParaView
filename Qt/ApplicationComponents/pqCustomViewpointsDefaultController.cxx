// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCustomViewpointsDefaultController.h"

#include "pqActiveObjects.h"
#include "pqCameraDialog.h"
#include "pqCustomViewpointsController.h"
#include "pqCustomViewpointsToolbar.h"
#include "pqRenderView.h"

//-----------------------------------------------------------------------------
pqCustomViewpointsDefaultController::pqCustomViewpointsDefaultController(QObject* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
QStringList pqCustomViewpointsDefaultController::getCustomViewpointToolTips()
{
  return pqCameraDialog::CustomViewpointToolTips();
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsDefaultController::configureCustomViewpoints()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    pqCameraDialog::configureCustomViewpoints(this->getToolbar(), view->getRenderViewProxy());
  }
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsDefaultController::setToCurrentViewpoint(int index)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    if (pqCameraDialog::setToCurrentViewpoint(index, view->getRenderViewProxy()))
    {
      view->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsDefaultController::applyCustomViewpoint(int index)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    if (pqCameraDialog::applyCustomViewpoint(index, view->getRenderViewProxy()))
    {
      view->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsDefaultController::deleteCustomViewpoint(int index)
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    if (pqCameraDialog::deleteCustomViewpoint(index, view->getRenderViewProxy()))
    {
      view->render();
    }
  }
}

//-----------------------------------------------------------------------------
void pqCustomViewpointsDefaultController::addCurrentViewpointToCustomViewpoints()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    pqCameraDialog::addCurrentViewpointToCustomViewpoints(view->getRenderViewProxy());
  }
}
