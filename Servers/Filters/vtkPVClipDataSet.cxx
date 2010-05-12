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
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkInformationStringVectorKey.h"

vtkStandardNewMacro(vtkPVClipDataSet);

//----------------------------------------------------------------------------
vtkPVClipDataSet::vtkPVClipDataSet(vtkImplicitFunction *vtkNotUsed(cf))
{
  // setting NumberOfOutputPorts to 1 because ParaView does not allow you to
  // generate the clipped output
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVClipDataSet::~vtkPVClipDataSet()
{
}

//----------------------------------------------------------------------------
void vtkPVClipDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::RequestData(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  vtkInformation * inInfo = inputVector[0]->GetInformationObject( 0 );

  if(!inInfo)
    {
    vtkErrorMacro( << "Failed to get input information.");
    return 0;
    }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());

  if(!inDataObj)
    {
    vtkErrorMacro( << "Failed to get input data object.");
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro( << "Failed to get output information.");
    }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!outDataObj)
    {
    vtkErrorMacro( << "Failed to get output data object.");
    }

  // Check if the input data is AMR.
  if(vtkHierarchicalBoxDataSet::SafeDownCast(inDataObj))
    {
    // If using scalars for clipping this should be NULL.
    if(!this->GetClipFunction())
      {
      vtkSmartPointer<vtkAMRDualClip> amrDC =
        vtkSmartPointer<vtkAMRDualClip>::New();
      amrDC->SetIsoValue(this->GetValue());

      // This is a lot to go through to get the name of the array to process.
      vtkInformationVector *inArrayVec =
        this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());
      if (!inArrayVec)
        {
        vtkErrorMacro("Problem finding array to process");
        return 0;
        }
      vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);
      if (!inArrayInfo)
        {
        vtkErrorMacro("Problem getting name of array to process.");
        return 0;
        }
      if ( ! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
        {
        vtkErrorMacro("Missing field name.");
        return 0;
        }
      const char *arrayNameToProcess =
        inArrayInfo->Get(vtkDataObject::FIELD_NAME());

      if(!arrayNameToProcess)
        {
        vtkErrorMacro("Unable to find valid array.");
        return 0;
        }

      amrDC->SetIsoValue(this->GetValue());

      // These default are safe to consider. Currently using GUI element just
      // for AMRDualClip filter enables all of these too.
      amrDC->SetEnableMergePoints(1);
      amrDC->SetEnableDegenerateCells(1);
      amrDC->SetEnableMultiProcessCommunication(1);

      amrDC->SetInput(0, inDataObj);
      amrDC->SetInputArrayToProcess(0, 0, 0, 0,
                                    arrayNameToProcess);
      amrDC->Update();

      vtkSmartPointer<vtkAppendFilter>
        append (vtkSmartPointer<vtkAppendFilter>::New());

      vtkSmartPointer<vtkCompositeDataIterator> itr;
      itr.TakeReference(vtkCompositeDataSet::SafeDownCast(
        amrDC->GetOutputDataObject(0))->NewIterator());
      itr->InitTraversal();

      bool found = false;
      while(!itr->IsDoneWithTraversal())
        {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(itr->GetCurrentDataObject());

        if(ds)
          {
          found = true;
          append->AddInput(ds);
          }

        itr->GoToNextItem();
        }

      if(found)
        {
        append->Update();
        }

      if(!append->GetOutput(0))
        {
        vtkErrorMacro("Unable to generate valid output.");
        return 0;
        }

      vtkUnstructuredGrid::SafeDownCast(outDataObj)->ShallowCopy(
        append->GetOutput(0));

      return 1;
      }
    else
      {
      vtkErrorMacro("This algorithm allows clipping using scalars only.");
      return 0;
      }
    }

  return Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkPVClipDataSet::FillInputPortInformation(int port,
                                               vtkInformation * info)
{
  this->Superclass::FillInputPortInformation(port, info);
  vtkInformationStringVectorKey::SafeDownCast(info->GetKey(
    vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))->Append(
    info, "vtkHierarchicalBoxDataSet");
  return 1;
}
