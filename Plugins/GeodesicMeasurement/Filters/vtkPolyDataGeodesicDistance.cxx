/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataGeodesicDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Copyright (c) 2013 Karthik Krishnan.
  Contributed to the VisualizationToolkit by the author under the terms
  of the Visualization Toolkit copyright

=========================================================================*/

#include "vtkPolyDataGeodesicDistance.h"

#include "vtkExecutive.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

vtkCxxSetObjectMacro(vtkPolyDataGeodesicDistance, Seeds, vtkIdList);

//-----------------------------------------------------------------------------
vtkPolyDataGeodesicDistance::vtkPolyDataGeodesicDistance()
{
  this->SetNumberOfInputPorts(1);
  this->FieldDataName = nullptr;
  this->Seeds = nullptr;
}

//-----------------------------------------------------------------------------
vtkPolyDataGeodesicDistance::~vtkPolyDataGeodesicDistance()
{
  this->SetFieldDataName(nullptr);
  this->SetSeeds(nullptr);
}

//-----------------------------------------------------------------------------
vtkFloatArray* vtkPolyDataGeodesicDistance::GetGeodesicDistanceField(vtkPolyData* pd)
{
  if (this->FieldDataName == nullptr)
  {
    return nullptr;
  }

  vtkDataArray* arr = pd->GetPointData()->GetArray(this->FieldDataName);
  if (vtkFloatArray* farr = vtkFloatArray::SafeDownCast(arr))
  {
    // Resize the existing one
    farr->SetNumberOfValues(pd->GetNumberOfPoints());
    if (!pd->GetPointData()->GetScalars())
    {
      pd->GetPointData()->SetScalars(farr);
    }
    return farr;
  }
  else if (!arr)
  {
    // Create a new one
    vtkFloatArray* farray = vtkFloatArray::New();
    farray->SetName(this->FieldDataName);
    farray->SetNumberOfValues(pd->GetNumberOfPoints());
    pd->GetPointData()->AddArray(farray);
    farray->Delete();
    if (!pd->GetPointData()->GetScalars())
    {
      pd->GetPointData()->SetScalars(farray);
    }
    return vtkFloatArray::SafeDownCast(pd->GetPointData()->GetArray(this->FieldDataName));
  }
  else
  {
    vtkErrorMacro(
      << "A array with a different datatype already exists with the same name on this polydata");
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
int vtkPolyDataGeodesicDistance::Compute()
{
  if (!this->Seeds || !this->Seeds->GetNumberOfIds())
  {
    vtkErrorMacro(<< "Please supply at least one seed.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkPolyDataGeodesicDistance::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime(), time;

  if (this->Seeds)
  {
    time = this->Seeds->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//-----------------------------------------------------------------------------
void vtkPolyDataGeodesicDistance::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Seeds)
  {
    os << indent << "Seeds: " << this->Seeds << endl;
    this->Seeds->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "FieldDataName: " << (this->FieldDataName ? this->FieldDataName : "None") << endl;
}
