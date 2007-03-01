/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentation1D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorRepresentation1D.h"

#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentation1D, "1.5");

vtkCxxSetObjectMacro(vtkTransferFunctionEditorRepresentation1D, Histogram,
                     vtkIntArray);

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation1D::vtkTransferFunctionEditorRepresentation1D()
{
  this->Histogram = NULL;
  this->HistogramImage = vtkImageData::New();
  this->HistogramImage->SetScalarTypeToUnsignedChar();
  this->DisplaySize[0] = this->DisplaySize[1] = 100;

  this->HistogramActor->SetInput(this->HistogramImage);
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation1D::~vtkTransferFunctionEditorRepresentation1D()
{
  this->SetHistogram(NULL);
  this->HistogramImage->Delete();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::SetDisplaySize(int x, int y)
{
  this->Superclass::SetDisplaySize(x, y);
  if (this->HistogramImage)
    {
    this->HistogramImage->Initialize();
    this->HistogramImage->SetDimensions(this->DisplaySize[0],
                                        this->DisplaySize[1], 1);
    this->HistogramImage->SetNumberOfScalarComponents(4);
    this->HistogramImage->AllocateScalars();
    vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(
      this->HistogramImage->GetPointData()->GetScalars());
    if (array)
      {
      array->FillComponent(0, 0);
      array->FillComponent(1, 0);
      array->FillComponent(2, 0);
      array->FillComponent(3, 0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::UpdateHistogramImage()
{
  if (!this->Histogram)
    {
    return;
    }

  vtkUnsignedCharArray *scalars = static_cast<vtkUnsignedCharArray*>(
    this->HistogramImage->GetPointData()->GetScalars());

  int numBins, minBinIdx;
  int maxBinIdx = this->Histogram->GetNumberOfTuples() - 1;

  if (this->ScalarBinRange[0] == 1 && this->ScalarBinRange[1] == 0)
    {
    numBins = this->Histogram->GetNumberOfTuples();
    minBinIdx = 0;
    }
  else
    {
    numBins = this->ScalarBinRange[1] - this->ScalarBinRange[0];
    minBinIdx = this->ScalarBinRange[0];
    }

  double range[2];
  this->Histogram->GetRange(range);
  double logRange = log(range[1]);

  int i, j, histogramIdx, height;

  for (i = 0; i < this->DisplaySize[0]; i++)
    {
    histogramIdx = vtkMath::Floor(i * numBins / this->DisplaySize[0]);
    histogramIdx += minBinIdx;
    if (histogramIdx < 0 || histogramIdx > maxBinIdx)
      {
      height = 0;
      }
    else
      {
      height = vtkMath::Floor(
        log((double)(this->Histogram->GetValue(histogramIdx))) *
        this->DisplaySize[1] / logRange);
      }

    for (j = 0; j < height; j++)
      {
      scalars->SetComponent(j * this->DisplaySize[0] + i, 0, 200);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 1, 200);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 2, 200);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 3, 255);
      }
    for (j = height; j < this->DisplaySize[1]; j++)
      {
      scalars->SetComponent(j * this->DisplaySize[0] + i, 0, 0);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 1, 0);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 2, 0);
      scalars->SetComponent(j * this->DisplaySize[0] + i, 3, 0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::BuildRepresentation()
{
  this->UpdateHistogramImage();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
