/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIsoVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIsoVolume.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkGenericClip.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPVClipDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkIsoVolume);

//----------------------------------------------------------------------------
// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper
vtkIsoVolume::vtkIsoVolume()
{
  this->LowerThreshold         = 0.0;
  this->UpperThreshold         = 1.0;

  this->LowerBoundClipDS = 0;
  this->UpperBoundClipDS = 0;

  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkIsoVolume::~vtkIsoVolume()
{
  // Release memory.
  if(this->LowerBoundClipDS)
    {
    this->LowerBoundClipDS->Delete();
    }
  if(this->UpperBoundClipDS)
    {
    this->UpperBoundClipDS->Delete();
    }
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkIsoVolume::ThresholdBetween(double lower, double upper)
{
  if(this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkIsoVolume::RequestData(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector)
{
  // Get the input information.
  vtkInformation* inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get output information.
  vtkDataObject* inObj  = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* outObj = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Comman vars.
  vtkStdString  arrayName("");
  int           fieldAssociation (-1);
  double*       range (0);
  bool          usingLowerBoundClipDS (false);
  bool          usingUpperBoundClipDS (false);

  vtkSmartPointer<vtkDataObject> outObj1(0);
  vtkSmartPointer<vtkDataObject> outObj2(0);

  // Get the array name and field information.
  vtkInformationVector *inArrayVec =
    this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());

  vtkInformation* inArrayInfo  = inArrayVec->GetInformationObject(0);

  if(!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
    {
    vtkErrorMacro("Unable to get field association.");
    return 1;
    }
  fieldAssociation = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  if ( ! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
    {
    vtkErrorMacro("Missing field name.");
    return 1;
    }
  arrayName = vtkStdString(inArrayInfo->Get(vtkDataObject::FIELD_NAME()));

  // Here is the logic.
  // If(AMR) {pass through:}
  // else if(Composite} {iterate and pass thru only if condition is true.}
  // else if(vtkDataSet) {pass through:}

  // Check if the input data is vtkDataSet or AMR or Multiblock.
  vtkHierarchicalBoxDataSet* hbds = vtkHierarchicalBoxDataSet::SafeDownCast(
    inObj);

  vtkDataSet* ds =vtkDataSet::SafeDownCast(inObj);

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(inObj);

  if(this->LowerBoundClipDS)
    {
    this->LowerBoundClipDS->Delete();
    this->LowerBoundClipDS = 0;
    }

  if(this->UpperBoundClipDS)
    {
    this->UpperBoundClipDS->Delete();
    this->UpperBoundClipDS = 0;
    }


  if(hbds || ds)
    {
    outObj1.TakeReference(outObj->NewInstance());

    usingLowerBoundClipDS = true;

    if(!this->LowerBoundClipDS)
      {
      this->LowerBoundClipDS = vtkPVClipDataSet::New();
      }

    this->LowerBoundClipDS->SetInput(0, inObj);
    this->LowerBoundClipDS->SetInputArrayToProcess(0, 0, 0, fieldAssociation,
                                                   arrayName);
    this->LowerBoundClipDS->SetValue(this->LowerThreshold);
    this->LowerBoundClipDS->Update();
    outObj1->ShallowCopy(this->LowerBoundClipDS->GetOutputDataObject(0));
    }
  else if(cds)
    {
    outObj1.TakeReference(outObj->NewInstance());
    vtkCompositeDataSet* out1cds = vtkCompositeDataSet::SafeDownCast(outObj1);
    out1cds->CopyStructure(cds);

    vtkSmartPointer<vtkCompositeDataIterator> itr (0);
    itr.TakeReference(cds->NewIterator());

    // Iterate thru each dataset and process only if the condition is true.
    for(itr->InitTraversal(); !itr->IsDoneWithTraversal();
        itr->GoToNextItem())
      {
      vtkSmartPointer<vtkDataObject> obj(0);
      obj.TakeReference(itr->GetCurrentDataObject()->NewInstance());
      vtkDataSet* lds = vtkDataSet::SafeDownCast(itr->GetCurrentDataObject());
      range = lds->GetScalarRange();

      if(range[0] <= this->LowerThreshold)
        {
        usingLowerBoundClipDS = true;
        if(!this->LowerBoundClipDS)
          {
          this->LowerBoundClipDS  = vtkPVClipDataSet::New();
          }
        this->LowerBoundClipDS->SetInput(0, lds);
        this->LowerBoundClipDS->SetInputArrayToProcess(0, 0, 0, fieldAssociation,
                                                       arrayName);
        this->LowerBoundClipDS->SetValue(this->LowerThreshold);
        this->LowerBoundClipDS->Update();
        obj->ShallowCopy(this->LowerBoundClipDS->GetOutputDataObject(0));
        out1cds->SetDataSet(itr, obj);
        }
      else
        {
        out1cds->SetDataSet(itr, itr->GetCurrentDataObject());
        }
      } // End for
    } // End if(cds)
  else
    {
    vtkErrorMacro("Unable to handle this data type.");
    return 1;
    }

  // Check if the input to this filter is vtkDataSet or AMR or composite.
  if(!usingLowerBoundClipDS)
    {
    outObj1 = inObj;
    }

  vtkHierarchicalBoxDataSet* hbds2 = vtkHierarchicalBoxDataSet::SafeDownCast(
    outObj1);

  vtkDataSet* ds2 = vtkDataSet::SafeDownCast(outObj1);

  vtkCompositeDataSet* cds2 = vtkCompositeDataSet::SafeDownCast(
    outObj1);

  if(hbds2 || ds2)
    {
    usingUpperBoundClipDS = true;
    outObj2.TakeReference(outObj->NewInstance());

    if(!this->UpperBoundClipDS)
      {
      this->UpperBoundClipDS = vtkPVClipDataSet::New();
      }

    this->UpperBoundClipDS->SetInput(0, outObj1);
    this->UpperBoundClipDS->SetInputArrayToProcess(0, 0, 0, fieldAssociation,
                                                   arrayName);
    this->UpperBoundClipDS->SetValue(this->UpperThreshold);
    this->UpperBoundClipDS->SetInsideOut(1);
    this->UpperBoundClipDS->Update();
    outObj2->ShallowCopy(this->UpperBoundClipDS->GetOutputDataObject(0));
    }
  else if(cds2)
    {
    outObj2.TakeReference(outObj->NewInstance());
    vtkCompositeDataSet* out2cds = vtkCompositeDataSet::SafeDownCast(outObj2);
    out2cds->CopyStructure(cds2);

    vtkSmartPointer<vtkCompositeDataIterator> itr (0);
    itr.TakeReference(cds2->NewIterator());

    for(itr->InitTraversal(); !itr->IsDoneWithTraversal();
        itr->GoToNextItem())
      {
      vtkSmartPointer<vtkDataObject> obj(0);
      obj.TakeReference(itr->GetCurrentDataObject()->NewInstance());
      vtkDataSet* lds = vtkDataSet::SafeDownCast(itr->GetCurrentDataObject());

      if(!lds->GetCellData()->GetAbstractArray(arrayName) &&
         !lds->GetPointData()->GetAbstractArray(arrayName))
        {
        usingUpperBoundClipDS = false;
        continue;
        }

      usingUpperBoundClipDS = true;

      range = lds->GetScalarRange();

      if(range[1] >= this->UpperThreshold || !usingLowerBoundClipDS)
        {
        if(!this->UpperBoundClipDS)
          {
          this->UpperBoundClipDS  = vtkPVClipDataSet::New();
          }
        this->UpperBoundClipDS->SetInput(0, lds);
        this->UpperBoundClipDS->SetInputArrayToProcess(0, 0, 0, fieldAssociation,
                                                       arrayName);
        this->UpperBoundClipDS->SetValue(this->UpperThreshold);
        this->UpperBoundClipDS->SetInsideOut(1);
        this->UpperBoundClipDS->Update();
        obj->ShallowCopy(this->UpperBoundClipDS->GetOutputDataObject(0));
        out2cds->SetDataSet(itr, obj);
        }
      else
        {
        out2cds->SetDataSet(itr, itr->GetCurrentDataObject());
        }
      }
    }
  else
    {
    vtkErrorMacro("Unable to handle this data type.");
    return 1;
    }

  if(usingLowerBoundClipDS && !usingUpperBoundClipDS)
    {
    outObj->ShallowCopy(outObj1);
    return 1;
    }
  else
    {
    outObj->ShallowCopy(outObj2);
    return 1;
    }
}

//----------------------------------------------------------------------------
int vtkIsoVolume::FillInputPortInformation(int vtkNotUsed(port),
                                           vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkIsoVolume::FillOutputPortInformation(int vtkNotUsed(port),
                                            vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkIsoVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";

  (this->LowerBoundClipDS) ?
      os << indent << "LowerBoundClipDS: " << this->LowerBoundClipDS << "\n" :
      os << indent << "LowerBoundClipDS: " << "NULL" << "\n" ;

  (this->UpperBoundClipDS) ?
      os << indent << "UpperBoundClipDS: " << this->UpperBoundClipDS << "\n" :
      os << indent << "UpperBoundClipDS: " << "NULL" << "\n" ;
}

//----------------------------------------------------------------------------
int vtkIsoVolume::ProcessRequest(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkIsoVolume::RequestDataObject(vtkInformation* vtkNotUsed(request),
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkCompositeDataSet* input = vtkCompositeDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
    {
    vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
      {
      output = vtkMultiBlockDataSet::New();
      output->SetPipelineInformation(outInfo);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
      }
    return 1;
    }
  else
    {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
      {
      output = vtkUnstructuredGrid::New();
      output->SetPipelineInformation(outInfo);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
      }
    return 1;
    }
}
