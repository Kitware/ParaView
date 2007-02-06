/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetHistogramFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetHistogramFilter.h"

#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkDataSetHistogramFilter, "1.1");
vtkStandardNewMacro(vtkDataSetHistogramFilter);

//----------------------------------------------------------------------------
vtkDataSetHistogramFilter::vtkDataSetHistogramFilter()
{
  this->OutputSpacing = 1.0;
  this->OutputOrigin = 0.0;
  this->OutputExtent[0] = 0;
  this->OutputExtent[1] = 255;
  this->Component = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkDataSetHistogramFilter::~vtkDataSetHistogramFilter()
{
}

//----------------------------------------------------------------------------
template <class T>
void vtkDataSetHistogramFilterExecute(vtkDataSetHistogramFilter *self,
                                      T *inPtr, int numComponents,
                                      int numTuples,
                                      vtkImageData *outData, int *outPtr)
{
  double *origin, *spacing;
  int *extent, *outPtr2;

  origin = outData->GetOrigin();
  spacing = outData->GetSpacing();
  extent = outData->GetExtent();

  // Zero count in every bin
  memset((void *)outPtr, 0, (extent[1]-extent[0]+1)*sizeof(int));

  int i, outIdx;
  for (i = 0; i < numTuples; i++)
    {
    outPtr2 = outPtr;
    outIdx = (int)(((double)(*inPtr) - origin[0]) / spacing[0]);
    if (outIdx < extent[0] || outIdx > extent[1])
      {
      break;
      }
    outPtr2 += outIdx - extent[0];
    if (outPtr2)
      {
      ++(*outPtr2);
      }
    inPtr += numComponents;
    }
}

//----------------------------------------------------------------------------
int vtkDataSetHistogramFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataSet *inData = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray *inArray = this->GetInputArrayToProcess(0, inputVector);
  if (this->Component > inArray->GetNumberOfComponents()-1)
    {
    this->Component = 0;
    }
  void *inPtr = inArray->GetVoidPointer(this->Component);

  outData->SetExtent(outData->GetWholeExtent());
  outData->AllocateScalars();
  void *outPtr = outData->GetScalarPointer();

  if (outData->GetScalarType() != VTK_INT)
    {
    vtkErrorMacro("RequestData: out ScalarType " << outData->GetScalarType()
                  << " must be int\n");
    return 1;
    }

  switch (inArray->GetDataType())
    {
    vtkTemplateMacro(vtkDataSetHistogramFilterExecute(
                       this, (VTK_TT *)(inPtr),
                       inArray->GetNumberOfComponents(),
                       inArray->GetNumberOfTuples(),
                       outData, (int *)(outPtr)));
    default:
      vtkErrorMacro("RequestData: unknown data array type");
      return 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetHistogramFilter::RequestInformation (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int ext[6];
  ext[0] = this->OutputExtent[0];
  ext[1] = this->OutputExtent[1];
  ext[2] = ext[3] = ext[4] = ext[5] = 0;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  double origin[3];
  origin[0] = this->OutputOrigin;
  origin[1] = origin[2] = 0;
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
  double spacing[3];
  spacing[0] = this->OutputSpacing;
  spacing[1] = spacing[2] = 0;
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_INT, 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetHistogramFilter::RequestUpdateExtent (
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed( outputVector ))
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // Use the whole extent of the input as the update extent.
  int extent[6] = {0,-1,0,-1,0,-1};
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDataSetHistogramFilter::FillInputPortInformation(int port,
                                                        vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetHistogramFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
