/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiActorHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiActorHelper.h"

#include "vtkActor.h"
#include "vtkActorCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkMultiActorHelper, "1.1.2.2");
vtkStandardNewMacro(vtkMultiActorHelper);

//----------------------------------------------------------------------------
vtkMultiActorHelper::vtkMultiActorHelper() 
{
  this->Actors = vtkActorCollection::New();
}

//----------------------------------------------------------------------------
vtkMultiActorHelper::~vtkMultiActorHelper() 
{
  this->Actors->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::Rotate()
{
}
  
//----------------------------------------------------------------------------
void vtkMultiActorHelper::Zoom()
{
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::AddActor(vtkActor* actor)
{
  this->Actors->AddItem(actor);
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::UniformScale(double scaleFactor)
{
  vtkCollectionSimpleIterator cookie;
  this->Actors->InitTraversal(cookie);
  vtkActor* actor=0;
  while ( (actor=this->Actors->GetNextActor(cookie)))
    {
    double scale[3];
    scale[0] = scale[1] = scale[2] = scaleFactor;
    cout << "Transforming: " << actor << endl;
    double **rotate = NULL;
    this->Prop3DTransform(actor, 0, rotate, scale);
    }
  cout << "------------------" << endl;
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::Prop3DTransform(vtkActor *actor,
                                          int numRotation,
                                          double **rotate,
                                          double *scale)
{
  double *boxCenter = actor->GetOrigin();

  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  actor->GetMatrix(oldMatrix);
  
  double orig[3];
  actor->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  if (actor->GetUserMatrix() != NULL) 
    {
    newTransform->SetMatrix(actor->GetUserMatrix());
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
  
  if (actor->GetUserMatrix() != NULL) 
    {
    newTransform->GetMatrix(actor->GetUserMatrix());
    }
  else 
    {
    actor->SetPosition(newTransform->GetPosition());
    actor->SetScale(newTransform->GetScale());
    actor->SetOrientation(newTransform->GetOrientation());
    }
  oldMatrix->Delete();
  newTransform->Delete();
}

// //----------------------------------------------------------------------------
// void vtkMultiActorHelper::Rotate()
// {
//   if (this->CurrentRenderer == NULL)
//     {
//     return;
//     }
  
//   vtkRenderWindowInteractor *rwi = this->Interactor;
//   vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();

//   // Get the view up and view right vectors
//   double view_up[3], view_look[3], view_right[3];

//   cam->OrthogonalizeViewUp();
//   cam->ComputeViewPlaneNormal();
//   cam->GetViewUp(view_up);
//   vtkMath::Normalize(view_up);
//   cam->GetViewPlaneNormal(view_look);
//   vtkMath::Cross(view_up, view_look, view_right);
//   vtkMath::Normalize(view_right);
  
//   int *size = this->CurrentRenderer->GetRenderWindow()->GetSize();

//   double nxf = static_cast<double>(rwi->GetEventPosition()[0]) / size[0];
//   double nyf = static_cast<double>(rwi->GetEventPosition()[1]) / size[1];

//   double oxf = static_cast<double>(rwi->GetLastEventPosition()[0]) / size[0];
//   double oyf = static_cast<double>(rwi->GetLastEventPosition()[1]) / size[1];

//   if (nxf * nxf <= 1.0 && nyf * nyf <= 1.0 &&
//       oxf * oxf <= 1.0 && oyf * oyf <= 1.0)
//     {
//     double newXAngle = asin(nxf) * vtkMath::RadiansToDegrees();
//     double newYAngle = asin(nyf) * vtkMath::RadiansToDegrees();
//     double oldXAngle = asin(oxf) * vtkMath::RadiansToDegrees();
//     double oldYAngle = asin(oyf) * vtkMath::RadiansToDegrees();
    
//     double scale[3];
//     scale[0] = scale[1] = scale[2] = 1.0;

//     double **rotate = new double*[2];

//     rotate[0] = new double[4];
//     rotate[1] = new double[4];
    
//     rotate[0][0] = (newXAngle - oldXAngle)*6;
//     rotate[0][1] = view_up[0];
//     rotate[0][2] = view_up[1];
//     rotate[0][3] = view_up[2];
    
//     rotate[1][0] = (oldYAngle - newYAngle)*6;
//     rotate[1][1] = view_right[0];
//     rotate[1][2] = view_right[1];
//     rotate[1][3] = view_right[2];
    

//     vtkCollectionSimpleIterator cookie;
//     vtkActorCollection* actors = this->CurrentRenderer->GetActors();
//     actors->InitTraversal(cookie);
//     vtkActor* actor=0;
//     while ((actor = actors->GetNextActor(cookie)))
//       {
//       this->Prop3DTransform(actor, 2, rotate, scale);
//       }
//     cout << "Rotating" << endl;
    
//     delete [] rotate[0];
//     delete [] rotate[1];
//     delete [] rotate;
    
//     if (this->AutoAdjustCameraClippingRange)
//       {
//       this->CurrentRenderer->ResetCameraClippingRange();
//       }

//     rwi->Render();
//     }
// }
  
// //----------------------------------------------------------------------------
// void vtkMultiActorHelper::Pan()
// {
//   if (this->CurrentRenderer == NULL)
//     {
//     return;
//     }

//   vtkRenderWindowInteractor *rwi = this->Interactor;
  
//   vtkCollectionSimpleIterator cookie;
//   vtkActorCollection* actors = this->CurrentRenderer->GetActors();
//   actors->InitTraversal(cookie);
//   vtkActor* actor=0;
//   while ((actor=actors->GetNextActor(cookie)))
//     {
//     // Use initial center as the origin from which to pan
    
//     double *obj_center = actor->GetCenter();

//     double disp_obj_center[3], new_pick_point[4];
//     double old_pick_point[4], motion_vector[3];
    
//     this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], 
//                                 disp_obj_center);
    
//     this->ComputeDisplayToWorld((double)rwi->GetEventPosition()[0], 
//                                 (double)rwi->GetEventPosition()[1], 
//                                 disp_obj_center[2],
//                                 new_pick_point);
    
//     this->ComputeDisplayToWorld(double(rwi->GetLastEventPosition()[0]), 
//                                 double(rwi->GetLastEventPosition()[1]), 
//                                 disp_obj_center[2],
//                                 old_pick_point);
    
//     motion_vector[0] = new_pick_point[0] - old_pick_point[0];
//     motion_vector[1] = new_pick_point[1] - old_pick_point[1];
//     motion_vector[2] = new_pick_point[2] - old_pick_point[2];
    
//     if (actor->GetUserMatrix() != NULL)
//       {
//       vtkTransform *t = vtkTransform::New();
//       t->PostMultiply();
//       t->SetMatrix(actor->GetUserMatrix());
//       t->Translate(motion_vector[0], motion_vector[1], motion_vector[2]);
//       actor->GetUserMatrix()->DeepCopy(t->GetMatrix());
//       t->Delete();
//       }
//     else
//       {
//       actor->AddPosition(motion_vector[0],
//                          motion_vector[1],
//                          motion_vector[2]);
//       }
//     }

//   cout << "Panning" << endl;

//   if (this->AutoAdjustCameraClippingRange)
//     {
//     this->CurrentRenderer->ResetCameraClippingRange();
//     }

//   rwi->Render();
// }

// //----------------------------------------------------------------------------
// void vtkMultiActorHelper::UniformScale()
// {
//   if (this->CurrentRenderer == NULL)
//     {
//     return;
//     }
  
//   vtkRenderWindowInteractor *rwi = this->Interactor;

//   int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
 
//   double *center = this->CurrentRenderer->GetCenter();

//   double yf = (double)dy / (double)center[1] * this->MotionFactor;
//   double scaleFactor = pow((double)1.1, yf);
  
//   double **rotate = NULL;
  
//   double scale[3];
//   scale[0] = scale[1] = scale[2] = scaleFactor;
  
//   vtkCollectionSimpleIterator cookie;
//   vtkActorCollection* actors = this->CurrentRenderer->GetActors();
//   actors->InitTraversal(cookie);
//   vtkActor* actor=0;
//   while ((actor=actors->GetNextActor(cookie)))
//     {
//     this->Prop3DTransform(actor, 0, rotate, scale);
//     }
  
//   if (this->AutoAdjustCameraClippingRange)
//     {
//     this->CurrentRenderer->ResetCameraClippingRange();
//     }

//   cout << "Scaling" << endl;
//   rwi->Render();
// }

// //----------------------------------------------------------------------------
// void vtkMultiActorHelper::Prop3DTransform(vtkProp3D *prop3D,
//                                                             int numRotation,
//                                                             double **rotate,
//                                                             double *scale)
// {
//   double *boxCenter = 0;
//   if (this->UseObjectCenter)
//     {
//     boxCenter = prop3D->GetCenter();
//     }
//   else
//     {
//     boxCenter = prop3D->GetOrigin();
//     }

//   vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
//   prop3D->GetMatrix(oldMatrix);
  
//   double orig[3];
//   prop3D->GetOrigin(orig);
  
//   vtkTransform *newTransform = vtkTransform::New();
//   newTransform->PostMultiply();
//   if (prop3D->GetUserMatrix() != NULL) 
//     {
//     newTransform->SetMatrix(prop3D->GetUserMatrix());
//     }
//   else 
//     {
//     newTransform->SetMatrix(oldMatrix);
//     }
  
//   newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
//   for (int i = 0; i < numRotation; i++) 
//     {
//     newTransform->RotateWXYZ(rotate[i][0], rotate[i][1],
//                              rotate[i][2], rotate[i][3]);
//     }
  
//   if ((scale[0] * scale[1] * scale[2]) != 0.0) 
//     {
//     newTransform->Scale(scale[0], scale[1], scale[2]);
//     }
  
//   newTransform->Translate(boxCenter[0], boxCenter[1], boxCenter[2]);
  
//   // now try to get the composit of translate, rotate, and scale
//   newTransform->Translate(-(orig[0]), -(orig[1]), -(orig[2]));
//   newTransform->PreMultiply();
//   newTransform->Translate(orig[0], orig[1], orig[2]);
  
//   if (prop3D->GetUserMatrix() != NULL) 
//     {
//     newTransform->GetMatrix(prop3D->GetUserMatrix());
//     }
//   else 
//     {
//     prop3D->SetPosition(newTransform->GetPosition());
//     prop3D->SetScale(newTransform->GetScale());
//     prop3D->SetOrientation(newTransform->GetOrientation());
//     }
//   oldMatrix->Delete();
//   newTransform->Delete();
// }

//----------------------------------------------------------------------------
void vtkMultiActorHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

