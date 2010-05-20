/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyle.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInteractorStyle.h"

#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkCameraManipulator.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVInteractorStyle);

//-------------------------------------------------------------------------
vtkPVInteractorStyle::vtkPVInteractorStyle()
{
  this->UseTimers = 0;
  this->CameraManipulators = vtkCollection::New();
  this->Current = NULL;
  this->CenterOfRotation[0] = this->CenterOfRotation[1]
    = this->CenterOfRotation[2] = 0;
 }

//-------------------------------------------------------------------------
vtkPVInteractorStyle::~vtkPVInteractorStyle()
{
  this->CameraManipulators->Delete();
  this->CameraManipulators = NULL;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::RemoveAllManipulators()
{
  this->CameraManipulators->RemoveAllItems();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::AddManipulator(vtkCameraManipulator *m)
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
  vtkCameraManipulator *manipulator;

  // Must not be processing an interaction to start another.
  if (this->Current)
    {
    return;
    }

  // Get the renderer.
  if (!this->CurrentRenderer)
    {
    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                            this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == NULL)
      {
      return;
      }
    }

  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  while ((manipulator = (vtkCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    if (manipulator->GetButton() == button && 
        manipulator->GetShift() == shift &&
        manipulator->GetControl() == control)
      {
      this->Current = manipulator;
      this->Current->Register(this);
      this->InvokeEvent(vtkCommand::StartInteractionEvent);
      this->Current->SetCenter(this->CenterOfRotation);
      this->Current->StartInteraction();
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
    this->Current->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->Current->UnRegister(this);
    this->Current = NULL;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMouseMove()
{
  if (!this->CurrentRenderer)
    {
    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                            this->Interactor->GetEventPosition()[1]);
    }
  
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
  os << indent << "CenterOfRotation: "
    << this->CenterOfRotation[0] << ", "
    << this->CenterOfRotation[1] << ", "
    << this->CenterOfRotation[2] << endl;
  os << indent << "CameraManipulators: " << this->CameraManipulators << endl;

}
