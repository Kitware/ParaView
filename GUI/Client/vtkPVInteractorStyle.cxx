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
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraManipulator.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkPVInteractorStyle, "1.8");
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

//----------------------------------------------------------------------------
void vtkPVInteractorStyle::SetCenterOfRotation(float x, float y, float z)
{
  vtkPVCameraManipulator *m;

  vtkCollectionIterator *it = this->CameraManipulators->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    m = static_cast<vtkPVCameraManipulator*>(it->GetCurrentObject());
    m->SetCenter(x, y, z);
    it->GoToNextItem();
    }
  it->Delete();
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
  while ((manipulator = (vtkPVCameraManipulator*)
                        this->CameraManipulators->GetNextItemAsObject()))
    {
    if (manipulator->GetButton() == button && 
        manipulator->GetShift() == shift &&
        manipulator->GetControl() == control)
      {
      this->Current = manipulator;
      this->Current->Register(this);
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
    this->Interactor->Render();
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
  os << indent << "CameraManipulators: " << this->CameraManipulators << endl;
}
