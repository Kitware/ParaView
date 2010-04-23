/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageSlicer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImageSlicer.h"

#include "vtkExtentTranslator.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPVImageSlicer);
//----------------------------------------------------------------------------
vtkPVImageSlicer::vtkPVImageSlicer()
{
  this->Slice = 0;
  this->SliceMode = XY_PLANE;
}

//----------------------------------------------------------------------------
vtkPVImageSlicer::~vtkPVImageSlicer()
{
}

//----------------------------------------------------------------------------
// What do we want from the input?
int vtkPVImageSlicer::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (inInfo)
    {
    // The output WholeExtent for this filter is just a slice. In VTK world, we
    // ask the input for simply the slice i.e. pass what the downstream filter
    // requested to the upstream. However, that results in subtle bugs such as
    // Bug #8658. So, we always ask from the input the full data for this piece
    // as if no slicing was employed.
    vtkStreamingDemandDrivenPipeline* executive =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
    int piece = executive->GetUpdatePiece(outInfo);
    int numPieces = executive->GetUpdateNumberOfPieces(outInfo);
    int ghostLevel = executive->GetUpdateGhostLevel(outInfo);

    int inWholeExtent[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
      inWholeExtent);

    vtkExtentTranslator* translator = executive->GetExtentTranslator(0);
    translator->SetWholeExtent(inWholeExtent);
    translator->SetPiece(piece);
    translator->SetNumberOfPieces(numPieces);
    translator->SetGhostLevel(ghostLevel);
    translator->PieceToExtent();

    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      translator->GetExtent(), 6);

    //// We can handle anything.
    inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
    }
  return 1;
}

//----------------------------------------------------------------------------
// Tell what we are producing in our output.
int vtkPVImageSlicer::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int outWholeExt[6] = {-1, -1, -1, -1, -1, -1};
  if (!inInfo)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      outWholeExt, 6);
    return 1; 
    }

  int inWholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
    inWholeExtent);

  int dataDescription = vtkStructuredData::SetExtent(inWholeExtent, outWholeExt);
  if (vtkStructuredData::GetDataDimension(dataDescription) != 3)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      inWholeExtent, 6);
    return 1;
    }

  int dims[3];
  dims[0] = inWholeExtent[1]-inWholeExtent[0]+1;
  dims[1] = inWholeExtent[3]-inWholeExtent[2]+1;
  dims[2] = inWholeExtent[5]-inWholeExtent[4]+1;
  
  unsigned int slice = this->Slice;
  switch (this->SliceMode)
    {
  case YZ_PLANE:
    slice = (static_cast<int>(slice)>=dims[0])?dims[0]-1:slice;
    outWholeExt[0] = outWholeExt[1] = outWholeExt[0]+slice;
    break;

  case XZ_PLANE:
    slice = (static_cast<int>(slice)>=dims[1])?dims[1]-1:slice;
    outWholeExt[2] = outWholeExt[3] = outWholeExt[2]+slice;
    break;

  case XY_PLANE:
  default:
    slice = (static_cast<int>(slice)>=dims[2])?dims[2]-1:slice;
    outWholeExt[4] = outWholeExt[5] = outWholeExt[4]+slice;
    break;
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    outWholeExt, 6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVImageSlicer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::GetData(inInfo);
  vtkImageData *output = vtkImageData::GetData(outInfo);

  int outWholeExt[6] ;
  output->GetWholeExtent(outWholeExt);

  vtkImageData* clone = input->NewInstance();
  clone->ShallowCopy(input);

  vtkExtractVOI* voi = vtkExtractVOI::New();
  voi->SetVOI(outWholeExt);
  voi->SetInput(clone);
  voi->Update();

  output->ShallowCopy(voi->GetOutput());
  // vtkExtractVOI is not passing correct origin. Until that's fixed, I
  // will just use the input origin/spacing to compute the bounds.
  output->SetOrigin(input->GetOrigin());

  voi->Delete();
  clone->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageSlicer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


