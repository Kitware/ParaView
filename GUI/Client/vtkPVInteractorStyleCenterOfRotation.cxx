/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleCenterOfRotation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInteractorStyleCenterOfRotation.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkPVWorldPointPicker.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVWindow.h"
#include "vtkKWEntry.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderModule.h"
#include "vtkPVApplication.h"

vtkCxxRevisionMacro(vtkPVInteractorStyleCenterOfRotation, "1.11");
vtkStandardNewMacro(vtkPVInteractorStyleCenterOfRotation);

//-------------------------------------------------------------------------
vtkPVInteractorStyleCenterOfRotation::vtkPVInteractorStyleCenterOfRotation()
{
  this->RenderModule = 0;
  this->PVWindow = 0;
  this->UseTimers = 0;
  this->Picker = vtkPVWorldPointPicker::New();

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
}

//-------------------------------------------------------------------------
vtkPVInteractorStyleCenterOfRotation::~vtkPVInteractorStyleCenterOfRotation()
{
  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::SetPVWindow(vtkPVWindow* w)
{
  this->PVWindow = w;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::SetRenderModule(vtkPVRenderModule* w)
{
  this->RenderModule = w;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::OnLeftButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::OnLeftButtonUp()
{
  this->Pick();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::Pick()
{
  if ( ! this->CurrentRenderer)
    {
    return;
    }
  
  double center[3];
  
  if ( ! this->Picker->GetRenderModule())
    {
    this->Picker->SetRenderModule(this->RenderModule);
    }
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  this->Picker->Pick(x, y, 0.0, this->CurrentRenderer);
  this->Picker->GetPickPosition(center);
  this->SetCenter(center[0], center[1], center[2]);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::SetCenter(float x, float y, float z)
{
  vtkPVWindow *window = this->PVWindow;
  if (window)
    {
    window->GetCenterXEntry()->SetValue(x);
    window->GetCenterYEntry()->SetValue(y);
    window->GetCenterZEntry()->SetValue(z);
    window->CenterEntryCallback();
    }
  window->ChangeInteractorStyle(1);
  
  this->Center[0] = x;
  this->Center[1] = y;
  this->Center[2] = z;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderModule" << this->RenderModule << endl;
  os << indent << "PVWindow" << this->PVWindow << endl;
  
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1]
     << ", " << this->Center[2] << ")" << endl;
}
