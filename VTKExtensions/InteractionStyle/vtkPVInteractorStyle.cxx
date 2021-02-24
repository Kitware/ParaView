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
#include "vtkCameraManipulator.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkPVInteractorStyle);

//-------------------------------------------------------------------------
vtkPVInteractorStyle::vtkPVInteractorStyle()
{
  this->UseTimers = 0;
  this->CameraManipulators = vtkCollection::New();
  this->CurrentManipulator = nullptr;
  this->CenterOfRotation[0] = this->CenterOfRotation[1] = this->CenterOfRotation[2] = 0;
  this->RotationFactor = 1.0;
}

//-------------------------------------------------------------------------
vtkPVInteractorStyle::~vtkPVInteractorStyle()
{
  this->CameraManipulators->Delete();
  this->CameraManipulators = nullptr;
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::RemoveAllManipulators()
{
  this->CameraManipulators->RemoveAllItems();
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::AddManipulator(vtkCameraManipulator* m)
{
  this->CameraManipulators->AddItem(m);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnLeftButtonDown()
{
  this->OnButtonDown(1, this->Interactor->GetShiftKey(), this->Interactor->GetControlKey());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnMiddleButtonDown()
{
  this->OnButtonDown(2, this->Interactor->GetShiftKey(), this->Interactor->GetControlKey());
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnRightButtonDown()
{
  this->OnButtonDown(3, this->Interactor->GetShiftKey(), this->Interactor->GetControlKey());
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
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
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
      this->Interactor->GetEventPosition()[1], this->CurrentRenderer, this->Interactor);
  }
}

//-------------------------------------------------------------------------
vtkCameraManipulator* vtkPVInteractorStyle::FindManipulator(int button, int shift, int control)
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = nullptr;
  while ((manipulator = (vtkCameraManipulator*)this->CameraManipulators->GetNextItemAsObject()))
  {
    if (manipulator->GetButton() == button && manipulator->GetShift() == shift &&
      manipulator->GetControl() == control)
    {
      return manipulator;
    }
  }
  return nullptr;
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
  if (this->CurrentManipulator == nullptr)
  {
    return;
  }
  if (this->CurrentManipulator->GetButton() == button)
  {
    this->CurrentManipulator->OnButtonUp(this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1], this->CurrentRenderer, this->Interactor);
    this->CurrentManipulator->EndInteraction();
    this->InvokeEvent(vtkCommand::EndInteractionEvent);
    this->CurrentManipulator->UnRegister(this);
    this->CurrentManipulator = nullptr;
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
    this->FindPokedRenderer(
      this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  }

  if (this->CurrentManipulator)
  {
    this->CurrentManipulator->OnMouseMove(this->Interactor->GetEventPosition()[0],
      this->Interactor->GetEventPosition()[1], this->CurrentRenderer, this->Interactor);
    this->InvokeEvent(vtkCommand::InteractionEvent);
  }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnChar()
{
  vtkRenderWindowInteractor* rwi = this->Interactor;

  switch (rwi->GetKeyCode())
  {
    case 'Q':
    case 'q':
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
  if (!this->CurrentRenderer)
  {
    return;
  }

  vtkLight* light;

  vtkLightCollection* lights = this->CurrentRenderer->GetLights();
  vtkCamera* camera = this->CurrentRenderer->GetActiveCamera();

  lights->InitTraversal();
  light = lights->GetNextItem();
  if (!light)
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
  vtkCameraManipulator* manipulator = nullptr;
  while ((manipulator = (vtkCameraManipulator*)this->CameraManipulators->GetNextItemAsObject()))
  {
    manipulator->OnKeyDown(this->Interactor);
  }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::OnKeyUp()
{
  // Look for a matching camera interactor.
  this->CameraManipulators->InitTraversal();
  vtkCameraManipulator* manipulator = nullptr;
  while ((manipulator = (vtkCameraManipulator*)this->CameraManipulators->GetNextItemAsObject()))
  {
    manipulator->OnKeyUp(this->Interactor);
  }
}

void vtkPVInteractorStyle::Dolly(double fact)
{
  if (this->Interactor->GetControlKey())
  {
    vtkPVInteractorStyle::DollyToPosition(
      fact, this->Interactor->GetEventPosition(), this->CurrentRenderer);
  }
  else
  {
    this->Superclass::Dolly(fact);
  }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::DollyToPosition(double fact, int* position, vtkRenderer* renderer)
{
  vtkCamera* cam = renderer->GetActiveCamera();
  if (cam->GetParallelProjection())
  {
    int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    // Zoom relatively to the cursor
    int* aSize = renderer->GetRenderWindow()->GetSize();
    int w = aSize[0];
    int h = aSize[1];
    x0 = w / 2;
    y0 = h / 2;
    x1 = position[0];
    y1 = position[1];
    vtkPVInteractorStyle::TranslateCamera(renderer, x0, y0, x1, y1);
    cam->SetParallelScale(cam->GetParallelScale() / fact);
    vtkPVInteractorStyle::TranslateCamera(renderer, x1, y1, x0, y0);
  }
  else
  {
    // Zoom relatively to the cursor position
    double viewFocus[4], originalViewFocus[3], cameraPos[3], newCameraPos[3];
    double newFocalPoint[4], norm[3];

    // Move focal point to cursor position
    cam->GetPosition(cameraPos);
    cam->GetFocalPoint(viewFocus);
    cam->GetFocalPoint(originalViewFocus);
    cam->GetViewPlaneNormal(norm);

    vtkPVInteractorStyle::ComputeWorldToDisplay(
      renderer, viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);

    vtkPVInteractorStyle::ComputeDisplayToWorld(
      renderer, double(position[0]), double(position[1]), viewFocus[2], newFocalPoint);

    cam->SetFocalPoint(newFocalPoint);

    // Move camera in/out along projection direction
    cam->Dolly(fact);

    // Find new focal point
    cam->GetPosition(newCameraPos);

    double newPoint[3];
    newPoint[0] = originalViewFocus[0] + newCameraPos[0] - cameraPos[0];
    newPoint[1] = originalViewFocus[1] + newCameraPos[1] - cameraPos[1];
    newPoint[2] = originalViewFocus[2] + newCameraPos[2] - cameraPos[2];

    cam->SetFocalPoint(newPoint);
  }
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::TranslateCamera(
  vtkRenderer* renderer, int toX, int toY, int fromX, int fromY)
{
  vtkCamera* cam = renderer->GetActiveCamera();
  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];
  cam->GetFocalPoint(viewFocus);

  vtkPVInteractorStyle::ComputeWorldToDisplay(
    renderer, viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
  focalDepth = viewFocus[2];

  vtkPVInteractorStyle::ComputeDisplayToWorld(
    renderer, double(toX), double(toY), focalDepth, newPickPoint);
  vtkPVInteractorStyle::ComputeDisplayToWorld(
    renderer, double(fromX), double(fromY), focalDepth, oldPickPoint);

  // camera motion is reversed
  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];

  cam->GetFocalPoint(viewFocus);
  cam->GetPosition(viewPoint);
  cam->SetFocalPoint(
    motionVector[0] + viewFocus[0], motionVector[1] + viewFocus[1], motionVector[2] + viewFocus[2]);

  cam->SetPosition(
    motionVector[0] + viewPoint[0], motionVector[1] + viewPoint[1], motionVector[2] + viewPoint[2]);
}

//-------------------------------------------------------------------------
void vtkPVInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CenterOfRotation: " << this->CenterOfRotation[0] << ", "
     << this->CenterOfRotation[1] << ", " << this->CenterOfRotation[2] << endl;
  os << indent << "RotationFactor: " << this->RotationFactor << endl;
  os << indent << "CameraManipulators: " << this->CameraManipulators << endl;
}
