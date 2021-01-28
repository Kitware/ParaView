/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkZSpaceCamera.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkZSpaceCamera.h"
#include "vtkZSpaceSDKManager.h"

#include "vtkObjectFactory.h"

#include "vtkMatrix4x4.h"
#include "vtkPerspectiveTransform.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkZSpaceCamera);

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceCamera::GetModelViewTransformMatrix()
{
  vtkMatrix4x4* zSpaceViewMatrix = this->GetStereo()
    ? this->ZSpaceSDKManager->GetStereoViewMatrix(this->GetLeftEye())
    : this->ZSpaceSDKManager->GetCenterEyeViewMatrix();

  this->ViewTransform->SetMatrix(zSpaceViewMatrix);

  this->Transform->Identity();
  this->Transform->SetupCamera(this->Position, this->FocalPoint, this->ViewUp);

  this->ViewTransform->Concatenate(this->Transform->GetMatrix());

  return this->ViewTransform->GetMatrix();
}

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkZSpaceCamera::GetProjectionTransformMatrix(
  double vtkNotUsed(aspect), double vtkNotUsed(nearz), double vtkNotUsed(farz))
{
  vtkMatrix4x4* zSpaceProjectionMatrix = this->GetStereo()
    ? this->ZSpaceSDKManager->GetStereoProjectionMatrix(this->GetLeftEye())
    : this->ZSpaceSDKManager->GetCenterEyeProjectionMatrix();
  return zSpaceProjectionMatrix;
}
