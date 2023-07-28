// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
