/*=========================================================================

  Program:   ParaView
  Module:    vtkImageTransparencyFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageTransparencyFilter.h"

#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageTransparencyFilter);

//-----------------------------------------------------------------------------
vtkImageTransparencyFilter::vtkImageTransparencyFilter() = default;

//-----------------------------------------------------------------------------
vtkImageTransparencyFilter::~vtkImageTransparencyFilter() = default;

//-----------------------------------------------------------------------------
int vtkImageTransparencyFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // TODO - check that two inputs have been set

  // Do this single-threaded for now
  vtkInformation* whiteInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* blackInfo = inputVector[0]->GetInformationObject(1);

  vtkImageData* whiteImage =
    static_cast<vtkImageData*>(whiteInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData* blackImage =
    static_cast<vtkImageData*>(blackInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkImageData* output = static_cast<vtkImageData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  output->Initialize();
  output->CopyStructure(whiteImage);
  output->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  // Handle only unsigned char images for now
  const unsigned char* white;
  const unsigned char* black;
  unsigned char* out;
  vtkIdType whiteStep[3];
  vtkIdType blackStep[3];
  vtkIdType outStep[3];

  white = static_cast<const unsigned char*>(
    whiteImage->GetScalarPointerForExtent(whiteImage->GetExtent()));
  black = static_cast<const unsigned char*>(
    blackImage->GetScalarPointerForExtent(blackImage->GetExtent()));
  out = static_cast<unsigned char*>(output->GetScalarPointerForExtent(output->GetExtent()));

  whiteImage->GetIncrements(whiteStep[0], whiteStep[1], whiteStep[2]);
  blackImage->GetIncrements(blackStep[0], blackStep[1], blackStep[2]);
  output->GetIncrements(outStep[0], outStep[1], outStep[2]);

  int* extent = output->GetExtent();

  for (int i = extent[4]; i <= extent[5]; ++i)
  {
    const unsigned char* whiteRow = white;
    const unsigned char* blackRow = black;
    unsigned char* outRow = out;

    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      const unsigned char* whitePx = whiteRow;
      const unsigned char* blackPx = blackRow;
      unsigned char* outPx = outRow;

      for (int k = extent[0]; k <= extent[1]; ++k)
      {
        if (whitePx[0] == blackPx[0] && whitePx[1] == blackPx[1] && whitePx[2] == blackPx[2])
        {
          outPx[0] = whitePx[0];
          outPx[1] = whitePx[1];
          outPx[2] = whitePx[2];
          outPx[3] = 255;
        }
        else
        {
          // Some kind of translucency; use values from the black capture.
          // The opacity is the derived from the V difference of the HSV
          // colors.
          double whiteHSV[3];
          double blackHSV[3];

          vtkMath::RGBToHSV(whitePx[0] / 255., whitePx[1] / 255., whitePx[2] / 255., whiteHSV + 0,
            whiteHSV + 1, whiteHSV + 2);
          vtkMath::RGBToHSV(blackPx[0] / 255., blackPx[1] / 255., blackPx[2] / 255., blackHSV + 0,
            blackHSV + 1, blackHSV + 2);
          double alpha = 1. - (whiteHSV[2] - blackHSV[2]);

          outPx[0] = static_cast<unsigned char>(blackPx[0] / alpha);
          outPx[1] = static_cast<unsigned char>(blackPx[1] / alpha);
          outPx[2] = static_cast<unsigned char>(blackPx[2] / alpha);
          outPx[3] = static_cast<unsigned char>(255 * alpha);
        }

        whitePx += whiteStep[0];
        blackPx += blackStep[0];
        outPx += outStep[0];
      }

      whiteRow += whiteStep[1];
      blackRow += blackStep[1];
      outRow += outStep[1];
    }

    white += whiteStep[2];
    black += blackStep[2];
    out += outStep[2];
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkImageTransparencyFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkImageTransparencyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
