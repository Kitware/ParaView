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
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPythonView.h"
#include "vtkWindowToImageFilter.h"


vtkStandardNewMacro(vtkSMPythonViewProxy);

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::vtkSMPythonViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMPythonViewProxy::~vtkSMPythonViewProxy()
{
}

//----------------------------------------------------------------------------
vtkRenderer* vtkSMPythonViewProxy::GetRenderer()
{
  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(
    this->GetClientSideObject());
  return pv ? pv->GetRenderer() : NULL;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkSMPythonViewProxy::GetRenderWindow()
{
  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(
    this->GetClientSideObject());
  return pv ? pv->GetRenderWindow() : NULL;
}

//----------------------------------------------------------------------------
bool vtkSMPythonViewProxy::LastRenderWasInteractive()
{
  return false;
}

//----------------------------------------------------------------------------
vtkImageData * vtkSMPythonViewProxy::CaptureWindowInternal(int magnification)
{
  this->GetRenderWindow()->SwapBuffersOff();

  this->CreateVTKObjects();
  vtkPythonView* pv = vtkPythonView::SafeDownCast(
    this->GetClientSideObject());
  if (pv)
    {
    pv->SetMagnification(magnification);
    }

  this->StillRender();

  if (pv)
    {
    pv->SetMagnification(1);
    }

  vtkSmartPointer<vtkWindowToImageFilter> w2i =
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();

  // BUG #8715: We go through this indirection since the active connection needs
  // to be set during update since it may request re-renders if magnification >1.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << w2i.GetPointer() << "Update"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkProcessModule::CLIENT);

  this->GetRenderWindow()->SwapBuffersOn();

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  this->GetRenderWindow()->Frame();
  return capture;
}

//----------------------------------------------------------------------------
void vtkSMPythonViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
