/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalCacheFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalCacheFilter.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPointSet.h"

vtkStandardNewMacro(vtkTemporalCacheFilter);
vtkCxxRevisionMacro(vtkTemporalCacheFilter, "1.1");
vtkCxxSetObjectMacro(vtkTemporalCacheFilter, Controller, 
  vtkMultiProcessController);
//-----------------------------------------------------------------------------
vtkTemporalCacheFilter::vtkTemporalCacheFilter()
{
  this->CollectedData = 0;
  this->UseCache = 0;
  this->AttributeToCollect = vtkTemporalCacheFilter::POINT_DATA;
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
vtkTemporalCacheFilter::~vtkTemporalCacheFilter()
{
  this->ClearCache(); 
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void vtkTemporalCacheFilter::ClearCache()
{
  if (this->CollectedData)
    {
    this->CollectedData->ReleaseData();
    this->CollectedData->Delete();
    this->CollectedData = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkTemporalCacheFilter::InitializeCollection(vtkPointSet* input)
{
  this->ClearCache();

  vtkFieldData* data;
  switch (this->AttributeToCollect)
    {
  case vtkTemporalCacheFilter::POINT_DATA:
    data = input->GetPointData();
    break;

  case vtkTemporalCacheFilter::FIELD_DATA:
    data = input->GetFieldData();
    break;

  case vtkTemporalCacheFilter::CELL_DATA:
    data = input->GetCellData();
    break;

  default:
    vtkErrorMacro("Invalid AttributeToCollect " << this->AttributeToCollect
      << ". Using Point data.");
    data = input->GetPointData();
    }

  this->CollectedData = input->NewInstance();
  vtkPoints *points = vtkPoints::New();
  this->CollectedData->SetPoints(points);
  points->Delete();
  this->CollectedData->GetPointData()->CopyStructure(data);
}

//-----------------------------------------------------------------------------
void vtkTemporalCacheFilter::CollectAttributeData(double time)
{
  
  vtkPointSet* input = vtkPointSet::SafeDownCast(this->GetInput());
  if (!input)
    {
    vtkErrorMacro("Cannot collect without input.");
    return;
    }
  // Ensure that the input is upto date.
  input->Update();


  if (this->Controller && this->Controller->GetLocalProcessId() > 0)
    {
    // When running in multi-process mode, the data is expected 
    // only on the root node, hence, we do nothing on the satellites.
    vtkDebugMacro("Ignored CollectAttributeData(" << time << ") on "
      "satellites");
    return;
    }

  // data pointer might have changed, get the point again.
  input = vtkPointSet::SafeDownCast(this->GetInput());
  if (!input)
    {
    vtkErrorMacro("Cannot collect without input.");
    return;
    }

  
  vtkFieldData* data;
  switch (this->AttributeToCollect)
    {
  case vtkTemporalCacheFilter::POINT_DATA:
    data = input->GetPointData();
    break;

  case vtkTemporalCacheFilter::FIELD_DATA:
    data = input->GetFieldData();
    break;

  case vtkTemporalCacheFilter::CELL_DATA:
    data = input->GetCellData();
    break;

  default:
    vtkErrorMacro("Invalid AttributeToCollect " << this->AttributeToCollect
      << ". Using Point data.");
    data = input->GetPointData();
    }
  if (!data)
    {
    vtkErrorMacro("Nothing to collect.");
    return;
    }
  // If cache not initialized, initialize it.
  if (!this->CollectedData)
    {
    this->InitializeCollection(input);
    }
  if (this->CollectedData)
    {
    this->CollectedData->GetPoints()->InsertNextPoint(time, 0.0, 0.0);
    // Insert the 0th tuple from the data.
    this->CollectedData->GetPointData()->InsertNextTuple(0, data);
    }
}

//-----------------------------------------------------------------------------
int vtkTemporalCacheFilter::RequestData(
  vtkInformation *vtkNotUsed(request), vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->Controller && this->Controller->GetLocalProcessId() > 0)
    {
    // When in multi-process mode, only root node is expected to have
    // all the data, so we release the data on all the satellites.
    output->ReleaseData();
    }
  else
    {
    if (this->UseCache)
      {
      if (!this->CollectedData)
        {
        this->CollectAttributeData(0.0);
        }
      output->ShallowCopy(this->CollectedData);
      }
    else
      {
      // Else behave as a pass-thru filter.
      output->ShallowCopy(input);
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkTemporalCacheFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseCache: " << this->UseCache << endl;
  os << indent << "AttributeToCollect: " << this->AttributeToCollect << endl;
  os << indent << "Controller: " << this->Controller << endl;
}
