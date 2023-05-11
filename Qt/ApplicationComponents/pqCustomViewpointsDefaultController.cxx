/*=========================================================================

  Program: ParaView
  Module: pqCustomViewpointsDefaultController.cxx

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

#include "pqCustomViewpointsDefaultController.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCameraDialog.h"
#include "pqCustomViewpointButtonDialog.h"
#include "pqCustomViewpointsController.h"
#include "pqCustomViewpointsToolbar.h"
#include "pqRenderView.h"
#include "pqSettings.h"

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
