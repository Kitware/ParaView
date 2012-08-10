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



//----------------------------------------------------------------------------
class vtkPVImplicitPlaneRepresentation::vtkPVInternal
{
public:
  double* GetOriginalBounds(){ return OriginalBounds; }
  double* GetOriginalOrigin(){ return OriginalOrigin; }
  double* GetOriginalNormal(){ return OriginalNormal; }

  bool StoreOriginalBounds() const { return StoreOriginalInfo[0]; }
  bool StoreOriginalOrigin() const { return StoreOriginalInfo[1]; }
  bool StoreOriginalNormal() const { return StoreOriginalInfo[2]; }

  void SetOriginalBounds(double bounds[6]);
  void SetOriginalOrigin(double origin[3]);
  void SetOriginalNormal(double normal[3]);
  
  vtkPVInternal();
  bool HasStoredInfo() const;
  void ClearStoredInfo();

  double ScaledOrigin[3];
  double ScaledNormal[3];

protected:
  bool StoreOriginalInfo[3];
  double OriginalBounds[6];
  double OriginalOrigin[3];
  double OriginalNormal[3];
};

//----------------------------------------------------------------------------
vtkPVImplicitPlaneRepresentation::vtkPVInternal::vtkPVInternal()
{
  this->StoreOriginalInfo[0] = true;
  this->StoreOriginalInfo[1] = true;
  this->StoreOriginalInfo[2] = true;

  this->OriginalBounds[0] = -1;
  this->OriginalBounds[1] = 1;
  this->OriginalBounds[2] = -1;
  this->OriginalBounds[3] = 1;
  this->OriginalBounds[4] = -1;
  this->OriginalBounds[5] = 1;

  this->OriginalOrigin[0] = 0.0;
  this->OriginalOrigin[1] = 0.0;
  this->OriginalOrigin[2] = 0.0;

  this->OriginalNormal[0] = 0.0;
  this->OriginalNormal[1] = 0.0;
  this->OriginalNormal[2] = 0.0;

  this->ScaledOrigin[0] = 0.0;
  this->ScaledOrigin[1] = 0.0;
  this->ScaledOrigin[2] = 0.0;

  this->ScaledNormal[0] = 0.0;
  this->ScaledNormal[1] = 0.0;
  this->ScaledNormal[2] = 0.0;
}

//----------------------------------------------------------------------------
bool vtkPVImplicitPlaneRepresentation::vtkPVInternal::HasStoredInfo() const
{
  return this->StoreOriginalInfo[0] == false &&
         this->StoreOriginalInfo[1] == false &&
         this->StoreOriginalInfo[2] == false;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::vtkPVInternal::ClearStoredInfo()
{
  this->StoreOriginalInfo[0] = this->StoreOriginalInfo[1] = 
    this->StoreOriginalInfo[2] = true;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::vtkPVInternal::SetOriginalBounds(
  double bounds[6])
{
  this->OriginalBounds[0] = bounds[0];
  this->OriginalBounds[1] = bounds[1];
  this->OriginalBounds[2] = bounds[2];
  this->OriginalBounds[3] = bounds[3];
  this->OriginalBounds[4] = bounds[4];
  this->OriginalBounds[5] = bounds[5];  
  this->StoreOriginalInfo[0] = false;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::vtkPVInternal::SetOriginalOrigin(
  double origin[3])
{
  this->OriginalOrigin[0] = origin[0];
  this->OriginalOrigin[1] = origin[1];
  this->OriginalOrigin[2] = origin[2];
  this->StoreOriginalInfo[1] = false;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::vtkPVInternal::SetOriginalNormal(
  double normal[3])
{
  this->OriginalNormal[0] = normal[0];
  this->OriginalNormal[1] = normal[1];
  this->OriginalNormal[2] = normal[2];
  this->StoreOriginalInfo[2] = false;
}


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

  this->Internal = new vtkPVInternal();

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
  this->Transform->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::Reset()
{
  this->Internal->ClearStoredInfo();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransform(vtkTransform *transform)
{
  if (transform && this->Transform->GetInput() != transform)
    {    
    this->Transform->SetInput(transform);
    this->InverseTransform->Update();
    }
  else if ( this->Transform->GetInput() )
    {
    this->UpdatePlacement();
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::UpdateTransformLocation()
{

  this->PlaceTransformedWidget(this->Internal->GetOriginalBounds());
  double *oo = this->Internal->GetOriginalOrigin();
  double *on = this->Internal->GetOriginalNormal();
  this->SetTransformedOrigin(oo[0],oo[1],oo[2]);
  this->SetTransformedNormal(on[0],on[1],on[2]);  
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::ClearTransform()
{
  this->Transform->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransformedOrigin(double x, double y, double z)
{
  double in_point[4] = {x, y, z, 1};
  this->Internal->SetOriginalOrigin(in_point);    
  
  this->Transform->TransformPoint(in_point, in_point);
  this->Superclass::SetOrigin(in_point);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::SetTransformedNormal(double x, double y, double z)
{    
  double in_point[3]={x,y,z};  
  this->Internal->SetOriginalNormal(in_point);    
  
  this->Transform->TransformNormal(in_point,in_point);  
  this->Superclass::SetNormal(in_point);
}

//----------------------------------------------------------------------------
double* vtkPVImplicitPlaneRepresentation::GetTransformedNormal()
{
  double * norm = this->Superclass::GetNormal();
  this->InverseTransform->TransformNormal(norm,this->Internal->ScaledNormal);
  return this->Internal->ScaledNormal;
}

//----------------------------------------------------------------------------
double* vtkPVImplicitPlaneRepresentation::GetTransformedOrigin()
{
  double * origin = this->Superclass::GetOrigin();
  this->InverseTransform->TransformPoint(origin, this->Internal->ScaledOrigin);
  return this->Internal->ScaledOrigin;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneRepresentation::PlaceTransformedWidget(double bounds[6])
{
  double tempBounds[6];
  if (this->Internal->StoreOriginalBounds())
    {
    this->Internal->SetOriginalBounds(bounds);    
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
  os << indent << "Transform: ";
  this->Transform->Print(os);
}
