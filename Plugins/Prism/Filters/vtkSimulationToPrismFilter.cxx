// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSimulationToPrismFilter.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMergeVectorComponents.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSimulationToPrismFilter);

//------------------------------------------------------------------------------
vtkSimulationToPrismFilter::vtkSimulationToPrismFilter()
{
  this->XArrayName = nullptr;
  this->YArrayName = nullptr;
  this->ZArrayName = nullptr;
  this->AttributeType = vtkDataObject::CELL;
}

//------------------------------------------------------------------------------
vtkSimulationToPrismFilter::~vtkSimulationToPrismFilter()
{
  this->SetXArrayName(nullptr);
  this->SetYArrayName(nullptr);
  this->SetZArrayName(nullptr);
}

//------------------------------------------------------------------------------
void vtkSimulationToPrismFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XArrayName: " << (this->XArrayName ? this->XArrayName : "(nullptr)") << endl;
  os << indent << "YArrayName: " << (this->YArrayName ? this->YArrayName : "(nullptr)") << endl;
  os << indent << "ZArrayName: " << (this->ZArrayName ? this->ZArrayName : "(nullptr)") << endl;
  os << indent
     << "AttributeType: " << vtkDataObject::GetAssociationTypeAsString(this->AttributeType) << endl;
}

//------------------------------------------------------------------------------
int vtkSimulationToPrismFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  auto input = vtkPolyData::GetData(inputVector[0], 0);
  auto output = vtkPolyData::GetData(outputVector, 0);
  if (input == nullptr || output == nullptr)
  {
    vtkErrorMacro(<< "Invalid or missing input and/or output!");
    return 0;
  }
  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  // check that array names are set
  if (this->XArrayName == nullptr || this->YArrayName == nullptr || this->ZArrayName == nullptr)
  {
    vtkErrorMacro(<< "No array names were set!");
    return 1;
  }

  vtkFieldData* inFD = input->GetAttributesAsFieldData(this->AttributeType);

  // get the point-data arrays
  vtkDataArray* xFD = inFD->GetArray(this->XArrayName);
  vtkDataArray* yFD = inFD->GetArray(this->YArrayName);
  vtkDataArray* zFD = inFD->GetArray(this->ZArrayName);

  // check that the arrays were found
  if (xFD == nullptr || yFD == nullptr || zFD == nullptr)
  {
    if (xFD == nullptr)
    {
      vtkErrorMacro(<< "X array " << this->XArrayName << " not found!");
    }
    if (yFD == nullptr)
    {
      vtkErrorMacro(<< "Y array " << this->YArrayName << " not found!");
    }
    if (zFD == nullptr)
    {
      vtkErrorMacro(<< "Z array " << this->ZArrayName << " not found!");
    }
    return 1;
  }
  // check that all arrays are scalars
  if (xFD->GetNumberOfComponents() != 1 || yFD->GetNumberOfComponents() != 1 ||
    zFD->GetNumberOfComponents() != 1)
  {
    vtkErrorMacro(<< "The arrays must have scalar values!");
    return 1;
  }

  // create points
  vtkNew<vtkMergeVectorComponents> merge;
  merge->SetContainerAlgorithm(this);
  merge->SetInputData(input);
  merge->SetAttributeType(this->AttributeType);
  merge->SetXArrayName(this->XArrayName);
  merge->SetYArrayName(this->YArrayName);
  merge->SetZArrayName(this->ZArrayName);
  merge->SetOutputVectorName("Coordinates");
  merge->Update();
  auto outputMerge = vtkDataSet::SafeDownCast(merge->GetOutput());
  if (outputMerge == nullptr)
  {
    vtkErrorMacro(<< "Invalid or missing output from merge filter!");
    return 0;
  }
  auto coordinates =
    outputMerge->GetAttributesAsFieldData(this->AttributeType)->GetArray("Coordinates");

  // set points
  vtkNew<vtkPoints> points;
  points->SetData(coordinates);
  output->SetPoints(points);
  // pass vertices
  output->SetVerts(input->GetVerts());
  // pass point/cell data
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}
