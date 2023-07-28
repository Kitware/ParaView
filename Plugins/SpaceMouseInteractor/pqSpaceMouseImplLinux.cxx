// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSpaceMouseImplLinux.h"

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqRenderView.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

#include "vtkLogger.h"
#include "vtkMatrix4x4.h"
#include "vtkObject.h"
#include "vtkTransform.h"

#include <vector>

pqSpaceMouseImpl::pqSpaceMouseImpl() {}

pqSpaceMouseImpl::~pqSpaceMouseImpl() = default;

void pqSpaceMouseImpl::setActiveView(pqView* view)
{
  pqRenderView* rview = qobject_cast<pqRenderView*>(view);
  vtkSMRenderViewProxy* renPxy = rview ? rview->getRenderViewProxy() : nullptr;

  this->Camera = nullptr;
  if (renPxy)
  {
    // this->Enable3DNavigation();
    this->Camera = renPxy->GetActiveCamera();
    // if (this->Camera)
    // {
    //   pqCoreUtilities::connect(
    //     this->Camera, vtkCommand::ModifiedEvent, this, SLOT(cameraChanged()));
    // }
    // this->cameraChanged();
  }
  else
  {
    // this->Disable3DNavigation();
  }
}

void pqSpaceMouseImpl::cameraChanged() {}

void pqSpaceMouseImpl::render()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (view)
  {
    view->render();
  }
}
