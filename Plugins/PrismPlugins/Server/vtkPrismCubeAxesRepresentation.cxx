/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPrismCubeAxesRepresentation.h"

#include "vtkCubeAxesActor.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPrismCubeAxesRepresentation);
//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::vtkPrismCubeAxesRepresentation()
{
}

//----------------------------------------------------------------------------
vtkPrismCubeAxesRepresentation::~vtkPrismCubeAxesRepresentation()
{
}

//----------------------------------------------------------------------------
int vtkPrismCubeAxesRepresentation::RequestData(vtkInformation* info,
  vtkInformationVector** inputVector, vtkInformationVector* outVector)
{  
  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    vtkFieldData *fieldData = input->GetFieldData();
    if (fieldData->HasArray("XRange") &&
        fieldData->HasArray("YRange") &&
        fieldData->HasArray("ZRange"))
      {
      double bounds[2];
      //set the custom range to the actor
      vtkDataArray *range;
      range = fieldData->GetArray("XRange");
      bounds[0] = range->GetTuple1(0);
      bounds[1] = range->GetTuple1(1);
      this->CubeAxesActor->SetXAxisRange(bounds);

      range = fieldData->GetArray("YRange");
      bounds[0] = range->GetTuple1(0);
      bounds[1] = range->GetTuple1(1);
      this->CubeAxesActor->SetYAxisRange(bounds);

      range = fieldData->GetArray("ZRange");
      bounds[0] = range->GetTuple1(0);
      bounds[1] = range->GetTuple1(1);
      this->CubeAxesActor->SetZAxisRange(bounds);
      }
    if (fieldData->HasArray("XTitle") &&
        fieldData->HasArray("YTitle") &&
        fieldData->HasArray("ZTitle"))
      {
      vtkAbstractArray *title;
      title= fieldData->GetAbstractArray("XTitle");
      this->SetXTitle(title->GetVariantValue(0).ToString().c_str());

      title = fieldData->GetAbstractArray("YTitle");
      this->SetYTitle(title->GetVariantValue(0).ToString().c_str());

      title = fieldData->GetAbstractArray("ZTitle");
      this->SetZTitle(title->GetVariantValue(0).ToString().c_str());
      }
    }
  return this->Superclass::RequestData(info,inputVector,outVector);
}
//----------------------------------------------------------------------------
void vtkPrismCubeAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
