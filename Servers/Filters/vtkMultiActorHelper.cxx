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

vtkCxxRevisionMacro(vtkMultiActorHelper, "1.3");
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
void vtkMultiActorHelper::AddActor(vtkActor* actor)
{
  this->Actors->AddItem(actor);
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::Pan(double x, double y)
{
  vtkCollectionSimpleIterator cookie;
  this->Actors->InitTraversal(cookie);
  vtkActor* actor=0;
  while ((actor=this->Actors->GetNextActor(cookie)))
    {
    actor->AddPosition(x, y, 0);
    double* origin = actor->GetOrigin();
    actor->SetOrigin(origin[0]+x, origin[1]+y, origin[2]);
    }

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
    double *rotate = NULL;
    this->Prop3DTransform(actor, 0, rotate, scale);
    }
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::Rotate(double transform[8])
{
  vtkCollectionSimpleIterator cookie;
  this->Actors->InitTraversal(cookie);
  vtkActor* actor=0;
  while ( (actor=this->Actors->GetNextActor(cookie)))
    {
    double scale[3];
    scale[0] = scale[1] = scale[2] = 1;
    this->Prop3DTransform(actor, 2, transform, scale);
    }
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::Prop3DTransform(vtkActor *actor,
                                          int numRotation,
                                          double *rotate,
                                          double *scale)
{
  double *boxCenter = actor->GetOrigin();

  vtkMatrix4x4 *oldMatrix = vtkMatrix4x4::New();
  actor->GetMatrix(oldMatrix);
  
  double orig[3];
  actor->GetOrigin(orig);
  
  vtkTransform *newTransform = vtkTransform::New();
  newTransform->PostMultiply();
  newTransform->SetMatrix(oldMatrix);
  
  newTransform->Translate(-(boxCenter[0]), -(boxCenter[1]), -(boxCenter[2]));
  
  for (int i = 0; i < numRotation; i++) 
    {
    newTransform->RotateWXYZ(rotate[i*4], rotate[i*4+1],
                             rotate[i*4+2], rotate[i*4+3]);
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
  
  actor->SetPosition(newTransform->GetPosition());
  actor->SetScale(newTransform->GetScale());
  actor->SetOrientation(newTransform->GetOrientation());

  oldMatrix->Delete();
  newTransform->Delete();
}

//----------------------------------------------------------------------------
void vtkMultiActorHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

