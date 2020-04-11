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
#include "vtkCompositeDataPipeline.h"
#include "vtkGenericClip.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkPVClipDataSet.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <assert.h>

vtkStandardNewMacro(vtkIsoVolume);

//----------------------------------------------------------------------------
// Construct with lower threshold=0, upper threshold=1, and threshold
// function=upper
vtkIsoVolume::vtkIsoVolume()
{
  this->LowerThreshold = 0.0;
  this->UpperThreshold = 1.0;
}

//----------------------------------------------------------------------------
vtkIsoVolume::~vtkIsoVolume()
{
}

//----------------------------------------------------------------------------
// Criterion is cells whose scalars are between lower and upper thresholds.
void vtkIsoVolume::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
  {
    this->LowerThreshold = lower <= upper ? lower : upper;
    this->UpperThreshold = upper >= lower ? upper : lower;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkIsoVolume::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the input information.
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get output information.
  vtkDataObject* inObj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* outObj = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Common vars.
  std::string arrayName("");
  int fieldAssociation(-1);
  // double*       range (0);
  // bool          usingLowerBoundClipDS (false);
  // bool          usingUpperBoundClipDS (false);

  vtkSmartPointer<vtkDataObject> outObj1(0);

  // Get the array name and field information.
  vtkInformationVector* inArrayVec = this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());

  vtkInformation* inArrayInfo = inArrayVec->GetInformationObject(0);

  // FIXME: Use newly added API on vtkAlgorithm to get the field association.
  if (!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
  {
    vtkErrorMacro("Unable to get field association.");
    return 1;
  }
  fieldAssociation = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

  if (!inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
  {
    vtkErrorMacro("Missing field name.");
    return 1;
  }
  arrayName = std::string(inArrayInfo->Get(vtkDataObject::FIELD_NAME()));

  // FIXME: Currently, both clips are always run. As a performance improvement
  // we can avoid running one of the clips if not needed.
  vtkDataObject* inputClone = inObj->NewInstance();
  inputClone->ShallowCopy(inObj);
  outObj1.TakeReference(
    this->Clip(inputClone, this->LowerThreshold, arrayName.c_str(), fieldAssociation, false));
  inputClone->Delete();

  outObj1.TakeReference(
    this->Clip(outObj1, this->UpperThreshold, arrayName.c_str(), fieldAssociation, true));

  assert(outObj1->IsA(outObj->GetClassName()));
  outObj->ShallowCopy(outObj1);
  return 1;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkIsoVolume::Clip(
  vtkDataObject* input, double value, const char* array_name, int fieldAssociation, bool invert)
{
  vtkPVClipDataSet* clipper = vtkPVClipDataSet::New();
  vtkCompositeDataPipeline* executive = vtkCompositeDataPipeline::New();
  clipper->SetExecutive(executive);
  executive->Delete();

  // disable AMRDualClip since it does not generate scalars.
  clipper->UseAMRDualClipForAMROff();
  clipper->SetInputData(0, input);
  clipper->SetInputArrayToProcess(0, 0, 0, fieldAssociation, array_name);
  clipper->SetValue(value);
  clipper->SetInsideOut(invert ? 1 : 0);
  clipper->Update();

  vtkDataObject* output = clipper->GetOutputDataObject(0);
  // we don't shallow copy here since the output will be shallow-copied anyways
  // before being sent as the real output of this filter.
  output->Register(this);
  clipper->Delete();
  return output;
}

//----------------------------------------------------------------------------
void vtkIsoVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
}

//----------------------------------------------------------------------------
int vtkIsoVolume::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
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
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return 1;
  }
  else
  {
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
    }
    return 1;
  }
}
