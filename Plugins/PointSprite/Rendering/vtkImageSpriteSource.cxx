/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpriteSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkImageSpriteSource
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkImageSpriteSource.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

;
vtkStandardNewMacro(vtkImageSpriteSource)
;

//----------------------------------------------------------------------------
vtkImageSpriteSource::vtkImageSpriteSource()
{
  this->SetNumberOfInputPorts(0);
  this->Maximum = 255;
  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;
  this->StandardDeviation = 0.5;
  this->AlphaMethod = vtkImageSpriteSource::NONE;
  this->AlphaThreshold = 1;
}

//----------------------------------------------------------------------------
void vtkImageSpriteSource::SetWholeExtent(int xMin,
    int xMax,
    int yMin,
    int yMax,
    int zMin,
    int zMax)
{
  int modified = 0;

  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax;
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkImageSpriteSource::RequestInformation(vtkInformation * vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      this->WholeExtent, 6);
  if (this->AlphaMethod != vtkImageSpriteSource::NONE)
    {
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 2);
    }
  else
    {
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_UNSIGNED_CHAR, 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageSpriteSource::RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  unsigned char *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  double scaleX, scaleY, scaleZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int *outExt;
  double sum;
  double xContrib, yContrib, zContrib;
  double temp;
  unsigned long count = 0;
  unsigned long target;

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(outInfo->Get(
      vtkDataObject::DATA_OBJECT()));
  vtkImageData *data = this->AllocateOutputData(output);

  if (data->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("Execute: This source only outputs doubles");
    }

  outExt = data->GetExtent();

  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outPtr = static_cast<unsigned char*> (data->GetScalarPointer(outExt[0], outExt[2],
      outExt[4]));

  target = static_cast<unsigned long> ((maxZ + 1) * (maxY + 1) / 50.0);
  target++;

  // Loop through ouput pixels
  temp = 1.0 / (2.0 * this->StandardDeviation * this->StandardDeviation);
  scaleX = (maxX >0? 1.0/maxX: 0.0);
  scaleY = (maxY >0? 1.0/maxY: 0.0);
  scaleZ = (maxZ >0? 1.0/maxZ: 0.0);

  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    zContrib = (idxZ-maxZ/2.0) * scaleZ;
    zContrib = zContrib * zContrib;
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count % target))
        {
        this->UpdateProgress(count / (50.0 * target));
        }
      count++;
      yContrib = (idxY-maxY/2.0) * scaleY;
      yContrib = yContrib * yContrib;
      for (idxX = 0; idxX <= maxX; idxX++)
        {
        // Pixel operation
        sum = zContrib + yContrib;
        xContrib = (idxX-maxX/2.0) * scaleX;
        xContrib = xContrib * xContrib;
        sum = sum + xContrib;
        double value = this->Maximum * exp(-sum * temp);
        *outPtr = static_cast<unsigned char> (floor(value));
        outPtr++;
        if (this->AlphaMethod == vtkImageSpriteSource::PROPORTIONAL)
          {
          unsigned char alpha = static_cast<unsigned char> (floor(value));
          *outPtr = alpha;
          outPtr++;
          }
        else if (this->AlphaMethod == vtkImageSpriteSource::CLAMP)
          {
          unsigned char alpha = 255;
          if (static_cast<unsigned char> (floor(value))
              < this->AlphaThreshold)
            alpha = 0;
          *outPtr = alpha;
          outPtr++;
          }
        }
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }

  return 1;
}

void vtkImageSpriteSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum: " << this->Maximum << "\n";

  os << indent << "StandardDeviation: " << this->StandardDeviation << "\n";
}

