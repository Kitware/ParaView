/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballMultiActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleTrackballMultiActor.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkProp3D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkInteractorStyleTrackballMultiActor, "1.1.2.1");
vtkStandardNewMacro(vtkInteractorStyleTrackballMultiActor);

vtkCxxSetObjectMacro(vtkInteractorStyleTrackballMultiActor,Application,vtkPVApplication);

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballMultiActor::vtkInteractorStyleTrackballMultiActor() 
{
  this->MotionFactor    = 10.0;
  this->UseObjectCenter = 0;
  this->Application = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTrackballMultiActor::~vtkInteractorStyleTrackballMultiActor() 
{
  this->SetApplication(0);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnMouseMove() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_ROTATE:
      this->FindPokedRenderer(x, y);
      this->Rotate();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_DOLLY:
      this->FindPokedRenderer(x, y);
      this->Dolly();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_SPIN:
      this->FindPokedRenderer(x, y);
      this->Spin();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_USCALE:
      this->FindPokedRenderer(x, y);
      this->UniformScale();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnLeftButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->Interactor->GetShiftKey())
    {
    this->StartPan();
    }
  else if (this->Interactor->GetControlKey())
    {
    this->StartSpin();
    }
  else
    {
    this->StartRotate();
    }

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOn();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnLeftButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_PAN:
      this->EndPan();
      break;

    case VTKIS_SPIN:
      this->EndSpin();
      break;

    case VTKIS_ROTATE:
      this->EndRotate();
      break;
    }

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOff();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnMiddleButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->Interactor->GetControlKey())
    {
    this->StartDolly();
    }
  else
    {
    this->StartPan();
    }

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOn();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnMiddleButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_DOLLY:
      this->EndDolly();
      break;

    case VTKIS_PAN:
      this->EndPan();
      break;
    }

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOff();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnRightButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  this->StartUniformScale();

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOn();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::OnRightButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_USCALE:
      this->EndUniformScale();
      break;
    }

  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOff();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::Rotate()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;
  vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

  // Get the view up and view right vectors
  double view_up[3], view_look[3], view_right[3];

  cam->OrthogonalizeViewUp();
  cam->ComputeViewPlaneNormal();
  cam->GetViewUp(view_up);
  vtkMath::Normalize(view_up);
  cam->GetViewPlaneNormal(view_look);
  vtkMath::Cross(view_up, view_look, view_right);
  vtkMath::Normalize(view_right);
  
  int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();

  double nxf = static_cast<double>(rwi->GetEventPosition()[0]) / size[0];
  double nyf = static_cast<double>(rwi->GetEventPosition()[1]) / size[1];

  double oxf = static_cast<double>(rwi->GetLastEventPosition()[0]) / size[0];
  double oyf = static_cast<double>(rwi->GetLastEventPosition()[1]) / size[1];

  if (nxf * nxf <= 1.0 && nyf * nyf <= 1.0 &&
      oxf * oxf <= 1.0 && oyf * oyf <= 1.0)
    {
    double newXAngle = asin(nxf) * vtkMath::RadiansToDegrees();
    double newYAngle = asin(nyf) * vtkMath::RadiansToDegrees();
    double oldXAngle = asin(oxf) * vtkMath::RadiansToDegrees();
    double oldYAngle = asin(oyf) * vtkMath::RadiansToDegrees();
    
    double scale[3];
    scale[0] = scale[1] = scale[2] = 1.0;

    double **rotate = new double*[2];

    rotate[0] = new double[4];
    rotate[1] = new double[4];
    
    rotate[0][0] = (newXAngle - oldXAngle)*6;
    rotate[0][1] = view_up[0];
    rotate[0][2] = view_up[1];
    rotate[0][3] = view_up[2];
    
    rotate[1][0] = (oldYAngle - newYAngle)*6;
    rotate[1][1] = view_right[0];
    rotate[1][2] = view_right[1];
    rotate[1][3] = view_right[2];
    

    vtkCollectionSimpleIterator cookie;
    vtkActorCollection* actors = this->CurrentRenderer->GetActors();
    actors->InitTraversal(cookie);
    vtkActor* actor=0;
    while (actor=actors->GetNextActor(cookie))
      {
      this->Prop3DTransform(actor, 2, rotate, scale);
      }
    cout << "Rotating" << endl;
    
    delete [] rotate[0];
    delete [] rotate[1];
    delete [] rotate;
    
    if (this->AutoAdjustCameraClippingRange)
      {
      this->CurrentRenderer->ResetCameraClippingRange();
      }

    rwi->Render();
    }
}
  
//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::Spin()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  
  vtkCollectionSimpleIterator cookie;
  vtkActorCollection* actors = this->CurrentRenderer->GetActors();
  actors->InitTraversal(cookie);
  vtkActor* actor=0;
  while (actor=actors->GetNextActor(cookie))
    {
    // Use initial center as the origin from which to pan
    
    double *obj_center = actor->GetCenter();

    double disp_obj_center[3], new_pick_point[4];
    double old_pick_point[4], motion_vector[3];
    
    this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
                                disp_obj_center);
    
    this->ComputeDisplayToWorld((double)rwi->GetEventPosition()[0], 
                                (double)rwi->GetEventPosition()[1], 
                                disp_obj_center[2],
                                new_pick_point);
    
    this->ComputeDisplayToWorld(double(rwi->GetLastEventPosition()[0]), 
                                double(rwi->GetLastEventPosition()[1]), 
                                disp_obj_center[2],
                                old_pick_point);
    
    motion_vector[0] = new_pick_point[0] - old_pick_point[0];
    motion_vector[1] = new_pick_point[1] - old_pick_point[1];
    motion_vector[2] = new_pick_point[2] - old_pick_point[2];
    
    if (actor->GetUserMatrix() != NULL)
      {
      vtkTransform *t = vtkTransform::New();
      t->PostMultiply();
      t->SetMatrix(actor->GetUserMatrix());
      t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
      actor->GetUserMatrix()->DeepCopy(t->GetMatrix());
      t->Delete();
      }
    else
      {
      actor->AddPosition(motion_vector[0],
                         motion_vector[1],
                         motion_vector[2]);
      }
    }

  cout << "Panning" << endl;

  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::Dolly()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::UniformScale()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  vtkRenderWindowInteractor *rwi = this->Interactor;

  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
 
  double *center = this->CurrentRenderer->GetCenter();

  double yf = (double)dy / (double)center[1] * this->MotionFactor;
  double scaleFactor = pow((double)1.1, yf);
  
  double **rotate = NULL;
  
  double scale[3];
  scale[0] = scale[1] = scale[2] = scaleFactor;
  
  vtkCollectionSimpleIterator cookie;
  vtkActorCollection* actors = this->CurrentRenderer->GetActors();
  actors->InitTraversal(cookie);
  vtkActor* actor=0;
  while (actor=actors->GetNextActor(cookie))
    {
    this->Prop3DTransform(actor, 0, rotate, scale);
    }
  
  if (this->AutoAdjustCameraClippingRange)
    {
    this->CurrentRenderer->ResetCameraClippingRange();
    }

  cout << "Scaling" << endl;
  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackballMultiActor::Prop3DTransform(vtkProp3D *prop3D,
                                                            int numRotation,
                                                            double **rotate,
                                                            double *scale)
{
  double *boxCenter = 0;
  if (this->UseObjectCenter)
    {
    boxCenter = prop3D->GetCenter();
    }
  else
    {
    boxCenter = prop3D->GetOrigin();
    }

  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  prop3D->GetMatrix(oldMatrix);
  
  double orig[3];
  prop3D->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->SetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    newTransform->SetMatrix(oldMatrix);
    }
  
  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
  for (int i = 0; i < numRotation; i++) 
    {
    newTransform->RotateWXYZ(rotate[i][0], rotate[i][1],
                             rotate[i][2], rotate[i][3]);
    }
  
  if ((scale[0] * scale[1] * scale[2]) != 0.0) 
    {
    newTransform->Scale(scale[0], scale[1], scale[2]);
    }
  
  newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);
  
  // now try to get the composit of translate, rotate, and scale
  newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
  newTransform->PreMultiply();
  newTransform->Translate(orig[0], orig[1], orig[2]);
  
  if (prop3D->GetUserMatrix() != NULL) 
    {
    newTransform->GetMatrix(prop3D->GetUserMatrix());
    }
  else 
    {
    prop3D->SetPosition(newTransform->GetPosition());
    prop3D->SetScale(newTransform->GetScale());
    prop3D->SetOrientation(newTransform->GetOrientation());
    }
  oldMatrix->Delete();
  newTransform->Delete();
}

