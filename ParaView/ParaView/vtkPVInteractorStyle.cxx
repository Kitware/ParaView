/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyle.cxx
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
#include "vtkPVInteractorStyle.h"


#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPVCameraManipulator.h"
#include "vtkCollection.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"

vtkCxxRevisionMacro(vtkPVInteractorStyle, "1.1");
vtkStandardNewMacro(vtkPVInteractorStyle);

//-------------------------------------------------------------------------
vtkPVInteractorStyle::vtkPVInteractorStyle()
{
  this->UseTimers = 0;
  this->CameraManipulators = vtkCollection::New();
  this->Current = NULL;
 }

//-------------------------------------------------------------------------
vtkPVInteractorStyle::~vtkPVInteractorStyle()
{
  this->CameraManipulators->Delete();
  this->CameraManipulators = NULL;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::AddManipulator(vtkPVCameraManipulator *m)
{
  this->CameraManipulators->AddItem(m);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnLeftButtonDown()
{
  this->OnButtonDown(1, this->Interactor->GetShiftKey(), 
		     this->Interactor->GetControlKey());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMiddleButtonDown()
{
  this->OnButtonDown(2, this->Interactor->GetShiftKey(), 
		     this->Interactor->GetControlKey());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnRightButtonDown()
{
  this->OnButtonDown(3, this->Interactor->GetShiftKey(), 
		     this->Interactor->GetControlKey());
}


//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnButtonDown(int button, int shift, int control)
{
  vtkPVCameraManipulator *manipulator;

  // Must not be processing an interaction to start another.
  if (this->Current)
    {
    return;
    }

  // Get the renderer.
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  

  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  while ((manipulator = (vtkPVCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    if (manipulator->GetButton() == button && 
        manipulator->GetShift() == shift &&
	manipulator->GetControl() == control)
      {
      this->Current = manipulator;
      this->Current->Register(this);
      this->Current->OnButtonDown(this->Interactor->GetEventPosition()[0],
                                  this->Interactor->GetEventPosition()[1],
                                  this->CurrentRenderer,
                                  this->Interactor);
      return;
      }
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnLeftButtonUp()
{
  this->OnButtonUp(1);
}
//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMiddleButtonUp()
{
  this->OnButtonUp(2);
}
//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnRightButtonUp()
{
  this->OnButtonUp(3);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnButtonUp(int button)
{
  if (this->Current == NULL)
    {
    return;
    }
  if (this->Current->GetButton() == button)
    {
    this->Current->OnButtonUp(this->Interactor->GetEventPosition()[0],
                              this->Interactor->GetEventPosition()[1],
                              this->CurrentRenderer,
                              this->Interactor);
    this->Current->UnRegister(this);
    this->Current = NULL;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMouseMove()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);

  if (this->Current)
    {
    this->Current->OnMouseMove(this->Interactor->GetEventPosition()[0],
                               this->Interactor->GetEventPosition()[1],
                               this->CurrentRenderer,
                               this->Interactor);
    }
}


//-------------------------------------------------------------------------
void vtkPVInteractorStyle::ResetLights()
{
  if ( ! this->CurrentRenderer)
    {
    return;
    }
  
  vtkLight *light;
  
  vtkLightCollection *lights = this->CurrentRenderer->GetLights();
  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  
  lights->InitTraversal();
  light = lights->GetNextItem();
  if ( ! light)
    {
    return;
    }
  light->SetPosition(camera->GetPosition());
  light->SetFocalPoint(camera->GetFocalPoint());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
