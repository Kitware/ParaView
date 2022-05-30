/*=========================================================================

   Program: ParaView
   Module:  pqSpaceMouseImpl.cxx

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
