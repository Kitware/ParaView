/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPivotManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPivotManipulator.h"

#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVWorldPointPicker.h"
#include "vtkRenderer.h"
#include "vtkPVRenderModule.h"

vtkStandardNewMacro(vtkPVPivotManipulator);
vtkCxxRevisionMacro(vtkPVPivotManipulator, "1.9");

//-------------------------------------------------------------------------
vtkPVPivotManipulator::vtkPVPivotManipulator()
{
  this->Picker = vtkPVWorldPointPicker::New();
  this->SetCenterOfRotation(0,0,0);
}

//-------------------------------------------------------------------------
vtkPVPivotManipulator::~vtkPVPivotManipulator()
{
  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::OnButtonDown(int, int, vtkRenderer *ren,
                                         vtkRenderWindowInteractor* rwi)
{
  if ( !this->Application )
    {
    vtkErrorMacro("Application is not defined");
    return;
    }
  if ( !ren ||!rwi )
    {
    vtkErrorMacro("Renderer or Render Window Interactor are not defined");
    return;
    }

  if ( ! this->Picker->GetRenderModule())
    {
    vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
    if ( !app )
      {
      return;
      }
    this->Picker->SetRenderModule(app->GetRenderModule());
    }
}


//-------------------------------------------------------------------------
void vtkPVPivotManipulator::OnButtonUp(int x, int y, vtkRenderer* ren,
                                       vtkRenderWindowInteractor*)
{
  this->Pick(ren, x, y);
  this->Picker->SetRenderModule(0);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::OnMouseMove(int x, int y, vtkRenderer* ren,
                                        vtkRenderWindowInteractor*)
{
  this->Pick(ren, x, y);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::Pick(vtkRenderer* ren, int x, int y)
{
  double center[3];
  
  
  this->Picker->Pick(x, y, 0.0, ren);
  this->Picker->GetPickPosition(center);
  this->SetCenterOfRotation(center[0], center[1], center[2]);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::SetCenterOfRotation(float* f)
{
  this->SetCenterOfRotation(f[0], f[1], f[2]);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::SetCenterOfRotation(float x, float y, float z)
{
  this->CenterOfRotation[0] = x;
  this->CenterOfRotation[1] = y;
  this->CenterOfRotation[2] = z;
  this->SetCenterOfRotationInternal(x,y,z);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::SetCenterOfRotationInternal(float x, float y, float z)
{
  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
  if ( !app )
    {
    return;
    }
  vtkPVWindow *window = app->GetMainWindow();
  if (window)
    {
    window->GetCenterXEntry()->SetValue(x);
    window->GetCenterYEntry()->SetValue(y);
    window->GetCenterZEntry()->SetValue(z);
    window->CenterEntryCallback();
    }
  const char* argument = "CenterOfRotation";
  void* calldata = const_cast<char*>(argument);
  this->InvokeEvent(vtkKWEvent::ManipulatorModifiedEvent, calldata);
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::SetResetCenterOfRotation()
{
  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
  if ( !app )
    {
    return;
    }
  vtkPVWindow *window = app->GetMainWindow();
  if (window)
    {
    window->ResetCenterCallback();
    float *center = this->GetCenter();
    this->SetCenterOfRotation(center);
    }
}

//-------------------------------------------------------------------------
void vtkPVPivotManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Center of rotation: " << this->CenterOfRotation[0] 
     << ", " << this->CenterOfRotation[1] << ", " << this->CenterOfRotation[2]
     << endl;
}






