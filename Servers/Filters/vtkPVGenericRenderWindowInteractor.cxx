/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkCommand.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkPVGenericRenderWindowInteractor, "1.2");
vtkCxxSetObjectMacro(vtkPVGenericRenderWindowInteractor,Renderer,vtkRenderer);

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::vtkPVGenericRenderWindowInteractor()
{
  this->PVRenderView = NULL;
  this->Renderer = NULL;
  this->InteractiveRenderEnabled = 0;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::~vtkPVGenericRenderWindowInteractor()
{
  this->SetPVRenderView(NULL);
  this->SetRenderer(NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::ConfigureEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetPVRenderView(vtkPVRenderViewProxy *view)
{
  if (this->PVRenderView != view)
    {
    if(this->PVRenderView)
      {
      this->PVRenderView->UnRegister(this);
      }
    // to avoid circular references
    this->PVRenderView = view;
    if (this->PVRenderView != NULL)
      {
      this->PVRenderView->Register(this);
      this->SetRenderWindow(this->PVRenderView->GetRenderWindow());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetMoveEventInformationFlipY(int x, int y)
{
  this->SetEventInformationFlipY(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVGenericRenderWindowInteractor::FindPokedRenderer(int,int)
{
  if (this->Renderer == NULL)
    {
    vtkErrorMacro("Renderer has not been set.");
    }

  return this->Renderer;
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::Render()
{
  if ( this->PVRenderView == NULL || this->RenderWindow == NULL)
    { // The case for interactors on the satellite processes.
    return;
    }

  // This should fix the problem of the plane widget render 
  if (this->InteractiveRenderEnabled)
    {
    this->PVRenderView->Render();
    }
  else
    {
    this->PVRenderView->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractiveRenderEnabled(int val)
{
  if (this->InteractiveRenderEnabled == val)
    {
    return;
    }
  this->Modified();
  this->InteractiveRenderEnabled = val;
  if (val == 0 && this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftPress(int x, int y, 
                                                     int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddlePress(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightPress(int x, int y, 
                                                      int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftRelease(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddleRelease(int x, int y, 
                                                         int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightRelease(int x, int y, 
                                                        int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMove(int x, int y) 
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, 
                            this->ControlKey, this->ShiftKey,
                            this->KeyCode, this->RepeatCount,
                            this->KeySym);
  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnKeyPress(char keyCode, int x, int y) 
{
  this->SetEventPosition(x, this->RenderWindow->GetSize()[1]-y);
  this->KeyCode = keyCode;
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << this->InteractiveRenderEnabled << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
}
