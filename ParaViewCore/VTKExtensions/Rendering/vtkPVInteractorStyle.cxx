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
  this->CurrentManipulator = NULL;
  this->CenterOfRotation[0] = this->CenterOfRotation[1]
    = this->CenterOfRotation[2] = 0;
  this->RotationFactor = 1.0;
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
  // Must not be processing an interaction to start another.
  if (this->CurrentManipulator)
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
  this->CurrentManipulator = this->FindManipulator(button, shift, control);
  if (this->CurrentManipulator)
    {
    this->CurrentManipulator->Register(this);
    this->InvokeEvent(vtkCommand::StartInteractionEvent);
    this->CurrentManipulator->SetCenter(this->CenterOfRotation);
    this->CurrentManipulator->SetRotationFactor(this->RotationFactor);
    this->CurrentManipulator->StartInteraction();
    this->CurrentManipulator->OnButtonDown(this->Interactor->GetEventPosition()[0],
                                this->Interactor->GetEventPosition()[1],
                                this->CurrentRenderer,
                                this->Interactor);
    }
}

//-------------------------------------------------------------------------
vtkCameraManipulator* vtkPVInteractorStyle::FindManipulator(int button, int shift, int control)
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = NULL;
  while ((manipulator = (vtkCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    if (manipulator->GetButton() == button &&
        manipulator->GetShift() == shift &&
        manipulator->GetControl() == control)
      {
      return manipulator;
      }
    }
  return NULL;
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
  if (this->CurrentManipulator == NULL)
    {
    return;
    }
  if (this->CurrentManipulator->GetButton() == button)
    {
    this->CurrentManipulator->OnButtonUp(this->Interactor->GetEventPosition()[0],
                              this->Interactor->GetEventPosition()[1],
                              this->CurrentRenderer,
                              this->Interactor);
    this->CurrentManipulator->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->CurrentManipulator->UnRegister(this);
    this->CurrentManipulator = NULL;
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMouseMove()
{
  if (this->CurrentRenderer && this->CurrentManipulator)
    {
    // When an interaction is active, we should not change the renderer being
    // interacted with.
    }
  else
    {
    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1]);
    }

  if (this->CurrentManipulator)
    {
    this->CurrentManipulator->OnMouseMove(this->Interactor->GetEventPosition()[0],
                               this->Interactor->GetEventPosition()[1],
                               this->CurrentRenderer,
                               this->Interactor);
    this->InvokeEvent(vtkCommand::InteractionEvent);
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnChar()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode())
    {
    case 'Q' :
    case 'q' :
      // It must be noted that this has no effect in QVTKInteractor and hence
      // we're assured that the Qt application won't exit because the user hit
      // 'q'.
      rwi->ExitCallback();
      break;
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
void vtkPVInteractorStyle::OnKeyDown()
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = NULL;
  while ((manipulator = (vtkCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    manipulator->OnKeyDown(this->Interactor);
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnKeyUp()
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = NULL;
  while ((manipulator = (vtkCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    manipulator->OnKeyUp(this->Interactor);
    }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CenterOfRotation: "
    << this->CenterOfRotation[0] << ", "
    << this->CenterOfRotation[1] << ", "
    << this->CenterOfRotation[2] << endl;
  os << indent << "RotationFactor: "<< this->RotationFactor << endl;
  os << indent << "CameraManipulators: " << this->CameraManipulators << endl;

}
