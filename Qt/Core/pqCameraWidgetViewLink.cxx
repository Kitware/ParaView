/*=========================================================================

  Program: ParaView
  Module:    pqCameraWidgetViewLink.cxx

  Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqCameraWidgetViewLink.h"

#include <QPointer>

#include "vtkCamera3DRepresentation.h"
#include "vtkCamera3DWidget.h"
#include "vtkCommand.h"
#include "vtkNew.h"

#include "pqRenderView.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkSMRenderViewProxy.h"

namespace
{
class vtkViewCallback : public vtkCommand
{
public:
  static vtkViewCallback* New() { return new vtkViewCallback; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long, void*) override
  {
    if (this->ViewProxy)
    {
      // Need to sync geometry bounds for client widget
      this->ViewProxy->SynchronizeGeometryBounds();
    }
  }

  vtkSMRenderViewProxy* ViewProxy = nullptr;

protected:
  vtkViewCallback() = default;
  ~vtkViewCallback() override = default;
};
}

struct pqCameraWidgetViewLink::pqInternal
{
  vtkNew<vtkCamera3DWidget> CameraWidget;
  vtkNew<vtkCamera3DRepresentation> CameraRepresentation;

  vtkNew<vtkViewCallback> Callback;
  QPointer<pqRenderView> DisplayView;
  QPointer<pqRenderView> LinkedView;
  vtkPVRenderView* LinkedPVView = nullptr;
};

//-----------------------------------------------------------------------------
pqCameraWidgetViewLink::pqCameraWidgetViewLink(pqRenderView* displayView, pqRenderView* linkedView)
  : Internal(new pqInternal)
{
  assert(displayView != nullptr && linkedView != nullptr);

  // Initialize View Pointers
  this->Internal->DisplayView = displayView;
  this->Internal->LinkedView = linkedView;
  this->Internal->LinkedPVView =
    vtkPVRenderView::SafeDownCast(linkedView->getViewProxy()->GetClientSideView());

  // Initialize widget and representation
  this->Internal->CameraWidget->SetInteractor(
    vtkSMRenderViewProxy::SafeDownCast(displayView->getViewProxy())->GetInteractor());
  this->Internal->CameraWidget->SetCurrentRenderer(
    vtkPVRenderView::SafeDownCast(displayView->getViewProxy()->GetClientSideView())->GetRenderer());

  this->Internal->CameraRepresentation->SetCamera(this->Internal->LinkedPVView->GetActiveCamera());
  this->Internal->CameraWidget->SetRepresentation(this->Internal->CameraRepresentation);
  this->Internal->CameraWidget->On();

  // Add view observers to update widget view bounds
  this->Internal->Callback->ViewProxy =
    vtkSMRenderViewProxy::SafeDownCast(this->Internal->DisplayView->getViewProxy());
  this->Internal->DisplayView->getViewProxy()->AddObserver(
    vtkCommand::PropertyModifiedEvent, this->Internal->Callback);
  this->Internal->LinkedView->getViewProxy()->AddObserver(
    vtkCommand::PropertyModifiedEvent, this->Internal->Callback);

  // Initial Render
  this->Internal->DisplayView->resetCamera();
  this->Internal->DisplayView->forceRender();
}

//-----------------------------------------------------------------------------
pqCameraWidgetViewLink::~pqCameraWidgetViewLink()
{
  if (this->Internal->DisplayView)
  {
    this->Internal->DisplayView->render();
  }
}

//-----------------------------------------------------------------------------
void pqCameraWidgetViewLink::saveXMLState(vtkPVXMLElement* xml)
{
  xml->AddAttribute("DisplayViewProxy",
    vtkSMRenderViewProxy::SafeDownCast(this->Internal->DisplayView->getViewProxy())->GetGlobalID());
  xml->AddAttribute("LinkedViewProxy",
    vtkSMRenderViewProxy::SafeDownCast(this->Internal->LinkedView->getViewProxy())->GetGlobalID());
}
