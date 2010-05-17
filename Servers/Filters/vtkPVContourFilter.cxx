/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVContourFilter.h"

#include "vtkAMRDualContour.h"
#include "vtkAppendPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPVContourFilter);


//-----------------------------------------------------------------------------
vtkPVContourFilter::vtkPVContourFilter() :
  vtkContourFilter()
{
}

//-----------------------------------------------------------------------------
vtkPVContourFilter::~vtkPVContourFilter()
{
}

//-----------------------------------------------------------------------------
void vtkPVContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::ProcessRequest(vtkInformation* request,
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

//-----------------------------------------------------------------------------
int vtkPVContourFilter::RequestData(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if(!inInfo)
    {
    vtkErrorMacro("Failed to get input information.");
    return 1;
    }

  vtkDataObject* inDataObj = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!inDataObj)
    {
    vtkErrorMacro("Failed to get input data object.");
    return 1;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro("Failed to get output information.");
    return 1;
    }

  vtkDataObject* outDataObj = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if(!outDataObj)
    {
    vtkErrorMacro("Failed get output data object.");
    return 1;
    }

  // Check if input is AMR data.
  if(vtkHierarchicalBoxDataSet::SafeDownCast(inDataObj))
    {
    // This is a lot to go through to get the name of the array to process.
     vtkInformationVector *inArrayVec =
       this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS());

     if(!inArrayVec)
       {
       vtkErrorMacro("Problem finding array to process");
       return 1;
       }

    vtkInformation *inArrayInfo = inArrayVec->GetInformationObject(0);

    if(!inArrayInfo)
      {
      vtkErrorMacro("Problem getting name of array to process.");
      return 1;
      }

    if(! inArrayInfo->Has(vtkDataObject::FIELD_NAME()))
      {
      vtkErrorMacro("Missing field name.");
      return 1;
      }

    const char* arrayNameToProcess =
      inArrayInfo->Get(vtkDataObject::FIELD_NAME());

    if(!arrayNameToProcess)
      {
      vtkErrorMacro("Unable to find valid array name.");
      return 1;
      }

    vtkSmartPointer<vtkAMRDualContour> amrDC(
      vtkSmartPointer<vtkAMRDualContour>::New());

    amrDC->SetInput(0, inDataObj);
    amrDC->SetInputArrayToProcess(0, 0, 0,
                                  vtkDataObject::FIELD_ASSOCIATION_CELLS,
                                  arrayNameToProcess);
    amrDC->SetEnableCapping(1);
    amrDC->SetEnableDegenerateCells(1);
    amrDC->SetEnableMultiProcessCommunication(1);
    amrDC->SetSkipGhostCopy(1);
    amrDC->SetTriangulateCap(1);
    amrDC->SetEnableMergePoints(1);

    for(int i=0; i < this->GetNumberOfContours(); ++i)
      {
      vtkSmartPointer<vtkMultiBlockDataSet> out (
        vtkSmartPointer<vtkMultiBlockDataSet>::New());
      amrDC->SetIsoValue(this->GetValue(i));
      amrDC->Update();
      out->ShallowCopy(amrDC->GetOutput(0));
      vtkMultiBlockDataSet::SafeDownCast(outDataObj)->SetBlock(i, out);
      }

    return 1;
    }
 else
   {
   return this->Superclass::RequestData(request, inputVector, outputVector);
   }
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::RequestDataObject(vtkInformation* request,
                                          vtkInformationVector** inputVector,
                                          vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::GetData(inInfo);
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
      output = vtkPolyData::New();
      output->SetPipelineInformation(outInfo);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
      output->Delete();
      }
    return 1;
    }
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::FillOutputPortInformation(int vtkNotUsed(port),
                                                  vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVContourFilter::FillInputPortInformation(int port,
                                                 vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // According to the documentation this is the way to append additional
  // input data set type since VTK 5.2.
  vtkInformationStringVectorKey::SafeDownCast(info->GetKey(
    vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE()))->Append(
      info, "vtkHierarchicalBoxDataSet");
  return 1;
}
