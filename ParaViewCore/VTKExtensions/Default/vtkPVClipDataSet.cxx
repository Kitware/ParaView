/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipDataSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClipDataSet.h"

#include "vtkAMRDualClip.h"
#include "vtkAppendFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkThreshold.h"
#include "vtkUnstructuredGrid.h"

#include "vtkInformationStringVectorKey.h"

#include <cassert>

vtkStandardNewMacro(vtkPVClipDataSet);

//----------------------------------------------------------------------------
vtkPVClipDataSet::vtkPVClipDataSet(vtkImplicitFunction* vtkNotUsed(cf))
{
  // setting NumberOfOutputPorts to 1 because ParaView does not allow you to
  // generate the clipped output
  this->SetNumberOfOutputPorts(1);

  this->UseAMRDualClipForAMR = true;
}

//----------------------------------------------------------------------------
vtkPVClipDataSet::~vtkPVClipDataSet()
{
}

//----------------------------------------------------------------------------
void vtkPVClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseAMRDualClipForAMR: " << this->UseAMRDualClipForAMR << endl;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (!inInfo)
  {
    vtkErrorMacro(<< "Failed to get input information.");
    return 0;
  }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!inDataObj)
  {
    vtkErrorMacro(<< "Failed to get input data object.");
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro(<< "Failed to get output information.");
  }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!outDataObj)
  {
    vtkErrorMacro(<< "Failed to get output data object.");
  }

  // Check if the input data is AMR and we are doing clip by cell scalars.
  if (vtkHierarchicalBoxDataSet::SafeDownCast(inDataObj))
  {
    // Using scalars.
    if (!this->GetClipFunction())
    {
      // This is a lot to go through to get the name of the array to process.
      vtkInformation* inArrayInfo = this->GetInputArrayInformation(0);
      int fieldAssociation(-1);
      if (!inArrayInfo->Has(vtkDataObject::FIELD_ASSOCIATION()))
      {
        vtkErrorMacro("Unable to query field association for the scalar.");
        return 1;
      }
      fieldAssociation = inArrayInfo->Get(vtkDataObject::FIELD_ASSOCIATION());

      if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
      {
        return this->ClipUsingSuperclass(request, inputVector, outputVector);
      }
      else if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS)
      {
        if (this->UseAMRDualClipForAMR)
        {
          vtkSmartPointer<vtkAMRDualClip> amrDC = vtkSmartPointer<vtkAMRDualClip>::New();
          amrDC->SetIsoValue(this->GetValue());

          // These default are safe to consider. Currently using GUI element just
          // for AMRDualClip filter enables all of these too.
          amrDC->SetEnableMergePoints(1);
          amrDC->SetEnableDegenerateCells(1);
          amrDC->SetEnableMultiProcessCommunication(1);

          vtkDataObject* inputClone = inDataObj->NewInstance();
          inputClone->ShallowCopy(inDataObj);
          amrDC->SetInputData(0, inputClone);
          inputClone->FastDelete();

          amrDC->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));
          amrDC->Update();
          outDataObj->ShallowCopy(amrDC->GetOutput(0));
        }
        else
        {
          return this->ClipUsingThreshold(request, inputVector, outputVector);
        }
        return 1;
      }
      else
      {
        vtkErrorMacro("Requires points or cell scalars.");
        return 1;
      }
    }
    else
    {
      return this->ClipUsingSuperclass(request, inputVector, outputVector);
    }
  }
  else // For vtkDataSet.
  {
    if (this->GetClipFunction())
    {
      return this->ClipUsingSuperclass(request, inputVector, outputVector);
    }

    vtkDataSet* ds(vtkDataSet::SafeDownCast(inDataObj));
    if (!ds)
    {
      vtkErrorMacro("Failed to get vtkDataSet.");
      return 1;
    }

    int association = this->GetInputArrayAssociation(0, ds);
    // If using point scalars.
    if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS)
    {
      return this->ClipUsingSuperclass(request, inputVector, outputVector);
    } // End if using point scalars.
    else if (association == vtkDataObject::FIELD_ASSOCIATION_CELLS)
    {
      // Use vtkPVThreshold here.
      return this->ClipUsingThreshold(request, inputVector, outputVector);
    }
    else
    {
      vtkErrorMacro("Unhandled association: " << association);
    }
  } // End for vtkDataSet.

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ClipUsingThreshold(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkSmartPointer<vtkThreshold> threshold(vtkSmartPointer<vtkThreshold>::New());

  vtkCompositeDataPipeline* executive = vtkCompositeDataPipeline::New();
  threshold->SetExecutive(executive);
  executive->FastDelete();

  vtkDataObject* inputClone = inputDO->NewInstance();
  inputClone->ShallowCopy(inputDO);
  threshold->SetInputData(0, inputClone);
  inputClone->FastDelete();
  threshold->SetInputArrayToProcess(0, this->GetInputArrayInformation(0));

  if (this->GetInsideOut())
  {
    threshold->ThresholdByLower(this->GetValue());
  }
  else
  {
    threshold->ThresholdByUpper(this->GetValue());
  }

  threshold->Update();
  outputDO->ShallowCopy(threshold->GetOutputDataObject(0));
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ClipUsingSuperclass(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector, 0);

  vtkCompositeDataSet* inputCD = vtkCompositeDataSet::SafeDownCast(inputDO);
  if (!inputCD)
  {
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkCompositeDataSet* outputCD = vtkCompositeDataSet::SafeDownCast(outputDO);

  outputCD->CopyStructure(inputCD);

  vtkSmartPointer<vtkHierarchicalBoxDataIterator> itr(0);
  itr.TakeReference(vtkHierarchicalBoxDataIterator::SafeDownCast(inputCD->NewIterator()));

  // Loop over all the datasets.
  for (itr->InitTraversal(); !itr->IsDoneWithTraversal(); itr->GoToNextItem())
  {
    // Creating new input information.
    vtkSmartPointer<vtkInformationVector> newInInfoVec =
      vtkSmartPointer<vtkInformationVector>::New();
    vtkSmartPointer<vtkInformation> newInInfo = vtkSmartPointer<vtkInformation>::New();
    newInInfo->Set(vtkDataObject::DATA_OBJECT(), itr->GetCurrentDataObject());
    newInInfoVec->SetInformationObject(0, newInInfo);

    // Creating new output information.
    vtkSmartPointer<vtkUnstructuredGrid> usGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    vtkSmartPointer<vtkInformationVector> newOutInfoVec =
      vtkSmartPointer<vtkInformationVector>::New();
    vtkSmartPointer<vtkInformation> newOutInfo = vtkSmartPointer<vtkInformation>::New();
    newOutInfo->Set(vtkDataObject::DATA_OBJECT(), usGrid);
    newOutInfoVec->SetInformationObject(0, newOutInfo);

    vtkInformationVector* newInInfoVecPtr = newInInfoVec.GetPointer();
    if (!this->Superclass::RequestData(request, &newInInfoVecPtr, newOutInfoVec.GetPointer()))
    {
      return 0;
    }
    outputCD->SetDataSet(itr, usGrid);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  vtkHierarchicalBoxDataSet* input = vtkHierarchicalBoxDataSet::GetData(inInfo);
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
      output->FastDelete();
    }
    return 1;
  }
  else
  {
    vtkDataSet* output = vtkDataSet::GetData(outInfo);
    if (!output)
    {
      output = vtkUnstructuredGrid::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->FastDelete();
    }
    return 1;
  }
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  vtkInformationStringVectorKey::SafeDownCast(
    info->GetKey(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))
    ->Append(info, "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}
