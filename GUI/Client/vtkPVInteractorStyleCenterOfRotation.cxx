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

vtkCxxRevisionMacro(vtkPVInteractorStyleCenterOfRotation, "1.9");
vtkStandardNewMacro(vtkPVInteractorStyleCenterOfRotation);

//-------------------------------------------------------------------------
vtkPVInteractorStyleCenterOfRotation::vtkPVInteractorStyleCenterOfRotation()
{
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
    vtkPVRenderView *view =
      ((vtkPVGenericRenderWindowInteractor*)this->Interactor)->GetPVRenderView();
    if (view)
      {
      this->Picker->SetRenderModule(view->GetPVApplication()->GetRenderModule());
      }
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
  vtkPVGenericRenderWindowInteractor *iren =
    vtkPVGenericRenderWindowInteractor::SafeDownCast(this->Interactor);
  vtkPVWindow *window =
    vtkPVWindow::SafeDownCast(iren->GetPVRenderView()->GetParentWindow());
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
  
  os << indent << "Center: (" << this->Center[0] << ", " << this->Center[1]
     << ", " << this->Center[2] << ")" << endl;
}
