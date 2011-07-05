/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImplicitPlaneRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkProperty.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkPVImplicitPlaneRepresentation);
//----------------------------------------------------------------------------
vtkPVImplicitPlaneRepresentation::vtkPVImplicitPlaneRepresentation()
{
  vtkMultiProcessController * ctrl = NULL;
  ctrl = vtkMultiProcessController::GetGlobalController();
  double opacity = 1;
  if(ctrl == NULL || ctrl->GetNumberOfProcesses() == 1)
    {
    opacity = 0.25;
    }

  this->OutlineTranslationOff();
  this->GetPlaneProperty()->SetOpacity(opacity);
  this->GetSelectedPlaneProperty()->SetOpacity(opacity);

  this->StoreOriginalInfo = true;

  this->OriginalBounds[0] = -1;
  this->OriginalBounds[1] = 1;
  this->OriginalBounds[2] = -1;
  this->OriginalBounds[3] = 1;
  this->OriginalBounds[4] = -1;
  this->OriginalBounds[5] = 1;

  this->OriginalOrigin[0] = 0.0;
  this->OriginalOrigin[1] = 0.0;
  this->OriginalOrigin[2] = 0.0;

  this->ScaledOrigin[0] = 0.0;
  this->ScaledOrigin[1] = 0.0;
  this->ScaledOrigin[2] = 0.0;

  this->ScaledNormal[0] = 0.0;
  this->ScaledNormal[1] = 0.0;
  this->ScaledNormal[2] = 0.0;

  //create and connect the two transforms together
  this->Transform = vtkTransform::New();
  this->InverseTransform = vtkTransform::New();
  this->Transform->PostMultiply();
  this->Transform->Identity();  
  this->InverseTransform->SetInput(this->Transform);
  this->InverseTransform->Inverse();
}

//----------------------------------------------------------------------------
vtkPVImplicitPlaneRepresentation::~vtkPVImplicitPlaneRepresentation()
{
  this->InverseTransform->Delete();
  this->Transform->Delete();;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransform(vtkTransform *transform)
{
  if (transform)
    {    
    this->Transform->SetInput(transform);
    this->InverseTransform->Update();

    this->StoreOriginalInfo = false;
    this->PlaceTransformedWidget(this->OriginalBounds);
    this->SetTransformedOrigin( this->OriginalOrigin[0],
                                this->OriginalOrigin[1],
                                this->OriginalOrigin[2]);
    this->StoreOriginalInfo = true;
    }
  else if ( this->Transform->GetInput() )
    {
    this->UpdatePlacement();
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::ClearTransform()
{
  this->StoreOriginalInfo = true;
  this->Transform->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransformedOrigin(double x, double y, double z)
{
  double in_point[4] = {x, y, z, 1};
  this->Transform->TransformPoint(in_point, in_point);
  this->Superclass::SetOrigin(in_point);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransformedNormal(double x, double y, double z)
{    
  double in_point[3]={x,y,z};
  this->Transform->TransformNormal(in_point,in_point);
  this->Superclass::SetNormal(in_point);
}

//----------------------------------------------------------------------------
double* vtkPVImplicitPlaneRepresentation::GetTransformedNormal()
{
  double * norm = this->Superclass::GetNormal();
  this->InverseTransform->TransformNormal(norm,this->ScaledNormal);
  return this->ScaledNormal;
}

//----------------------------------------------------------------------------
double* vtkPVImplicitPlaneRepresentation::GetTransformedOrigin()
{
  double * origin = this->Superclass::GetOrigin();
  this->InverseTransform->TransformPoint(origin, this->ScaledOrigin);
  return this->ScaledOrigin;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::PlaceTransformedWidget(double bounds[6])
{
  double tempBounds[6];
  if (this->StoreOriginalInfo)
    {
    this->OriginalBounds[0] = bounds[0];
    this->OriginalBounds[1] = bounds[1];
    this->OriginalBounds[2] = bounds[2];
    this->OriginalBounds[3] = bounds[3];
    this->OriginalBounds[4] = bounds[4];
    this->OriginalBounds[5] = bounds[5];

    this->OriginalOrigin[0] = (bounds[1] + bounds[0])/2;
    this->OriginalOrigin[1] = (bounds[2] + bounds[3])/2;
    this->OriginalOrigin[2] = (bounds[4] + bounds[5])/2;

    this->StoreOriginalInfo = false;
    }

  double point[3] = {bounds[0], bounds[2], bounds[4] };
  double point2[3] = {bounds[1], bounds[3], bounds[5] };  
  this->Transform->TransformPoint(point, point);
  tempBounds[0] = point[0];
  tempBounds[2] = point[1];
  tempBounds[4] = point[2];

  this->Transform->TransformPoint(point2, point2);
  tempBounds[1] = point2[0];
  tempBounds[3] = point2[1];
  tempBounds[5] = point2[2];

  this->Superclass::PlaceWidget(tempBounds);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StoreOriginalInfoFlag: " << this->StoreOriginalInfo << "\n";
  os << indent << "OriginalBounds: " << "( " << 
     this->OriginalBounds[0] << ", " << this->OriginalBounds[1] << ", " <<
     this->OriginalBounds[2] << ", " << this->OriginalBounds[3] << ", " <<
     this->OriginalBounds[4] << ", " << this->OriginalBounds[5] << ")\n";
  os << indent << "OriginalOrigin: " << "( " << 
     this->OriginalOrigin[0] << ", " << this->OriginalOrigin[1] << ", " <<
     this->OriginalOrigin[2] << ")\n";
  os << indent << "Transform: ";
  this->Transform->Print(os);
}
