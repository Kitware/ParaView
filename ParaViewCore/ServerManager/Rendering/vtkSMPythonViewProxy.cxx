/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPythonViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPythonViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPythonView.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMViewProxyInteractorHelper.h"
#include "vtkWindowToImageFilter.h"

vtkStandardNewMacro(vtkSMPythonViewProxy);

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::vtkSMPythonViewProxy()
{
  this->InteractorHelper->SetViewProxy(this);
}

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::~vtkSMPythonViewProxy()
{
  this->InteractorHelper->SetViewProxy(NULL);
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
  return pv ? pv->GetRenderer() : NULL;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMPythonViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(this->GetClientSideObject());
  return pv ? pv->GetRenderWindow() : NULL;
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMPythonViewProxy::CaptureWindowInternal(int magnification)
{
  vtkPythonView* pv = vtkPythonView::SafeDownCast(this->GetClientSideObject());
  if (pv)
  {
    pv->SetMagnification(magnification);
  }
  vtkImageData* image = this->Superclass::CaptureWindowInternal(magnification);
  if (pv)
  {
    pv->SetMagnification(1);
  }
  return image;
}

//----------------------------------------------------------------------------
void vtkSMPythonViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
