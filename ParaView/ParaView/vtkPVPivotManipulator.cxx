/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPivotManipulator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

vtkStandardNewMacro(vtkPVPivotManipulator);
vtkCxxRevisionMacro(vtkPVPivotManipulator, "1.2");

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

  if ( ! this->Picker->GetComposite())
    {
    vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
    if ( !app )
      {
      return;
      }
    vtkPVWindow *window = app->GetMainWindow();
    vtkPVRenderView *view = window->GetMainView();
    if (view)
      {
      this->Picker->SetComposite(view->GetComposite());
      }
    else
      {
      return;
      }
    }
}


//-------------------------------------------------------------------------
void vtkPVPivotManipulator::OnButtonUp(int x, int y, vtkRenderer* ren,
                                       vtkRenderWindowInteractor*)
{
  this->Pick(ren, x, y);
  this->Picker->SetComposite(0);
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
  float center[3];
  
  
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
    window->GetCenterXEntry()->SetValue(x, 3);
    window->GetCenterYEntry()->SetValue(y, 3);
    window->GetCenterZEntry()->SetValue(z, 3);
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
}






