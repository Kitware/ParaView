// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPythonViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPythonView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMViewProxyInteractorHelper.h"

vtkStandardNewMacro(vtkSMPythonViewProxy);

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::vtkSMPythonViewProxy()
{
  this->InteractorHelper->SetViewProxy(this);
}

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::~vtkSMPythonViewProxy()
{
  this->InteractorHelper->SetViewProxy(nullptr);
  this->InteractorHelper->CleanupInteractor();
}

//----------------------------------------------------------------------------
void vtkSMPythonViewProxy::SetupInteractor(vtkRenderWindowInteractor* iren)
{
  if (this->GetLocalProcessSupportsInteraction())
  {
    this->CreateVTKObjects();
    this->InteractorHelper->SetupInteractor(iren);
  }
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkSMPythonViewProxy::GetInteractor()
{
  this->CreateVTKObjects();
  return this->GetRenderWindow()->GetInteractor();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkSMPythonViewProxy::GetRenderer()
{
  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(this->GetClientSideObject());
  return pv ? pv->GetRenderer() : nullptr;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMPythonViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(this->GetClientSideObject());
  return pv ? pv->GetRenderWindow() : nullptr;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMPythonViewProxy::CaptureWindowInternal(int magX, int magY)
{
  vtkPythonView* pv = vtkPythonView::SafeDownCast(this->GetClientSideObject());
  if (pv)
  {
    pv->SetMagnification(magX, magY);
  }
  vtkImageData* image = this->Superclass::CaptureWindowInternal(magX, magY);
  if (pv)
  {
    pv->SetMagnification(1, 1);
  }
  return image;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMPythonViewProxy::PreRender(bool vtkNotUsed(interactive))
{
  return vtkPVSession::CLIENT;
}

//----------------------------------------------------------------------------
void vtkSMPythonViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
