/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleCenterOfRotation.cxx
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
#include "vtkPVInteractorStyleCenterOfRotation.h"

#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkPVWorldPointPicker.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVWindow.h"
#include "vtkKWEntry.h"
#include "vtkPVRenderView.h"

vtkCxxRevisionMacro(vtkPVInteractorStyleCenterOfRotation, "1.2");
vtkStandardNewMacro(vtkPVInteractorStyleCenterOfRotation);

//-------------------------------------------------------------------------
vtkPVInteractorStyleCenterOfRotation::vtkPVInteractorStyleCenterOfRotation()
{
  this->UseTimers = 0;
  this->Picker = vtkPVWorldPointPicker::New();
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
  
  float center[3];
  
  if ( ! this->Picker->GetComposite())
    {
    vtkPVRenderView *view =
      ((vtkPVGenericRenderWindowInteractor*)this->Interactor)->GetPVRenderView();
    if (view)
      {
      this->Picker->SetComposite(view->GetComposite());
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
    window->GetCenterXEntry()->SetValue(x, 3);
    window->GetCenterYEntry()->SetValue(y, 3);
    window->GetCenterZEntry()->SetValue(z, 3);
    window->CenterEntryCallback();
    }
  window->ChangeInteractorStyle(1);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyleCenterOfRotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
