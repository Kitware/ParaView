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

#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkUnsignedCharArray.h"


//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation1D::vtkTransferFunctionEditorRepresentation1D()
{
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation1D::~vtkTransferFunctionEditorRepresentation1D()
{
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::SetShowColorFunctionInHistogram(
  int color)
{
  this->Superclass::SetShowColorFunctionInHistogram(color);
  this->UpdateHistogramImage();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::UpdateHistogramImage()
{
  int upToDate = (this->HistogramImage->GetMTime() > this->GetMTime());
  if (this->ShowColorFunctionInHistogram)
    {
    upToDate &=
      (this->HistogramImage->GetMTime() > this->ColorFunction->GetMTime());
    }

  if (!this->HistogramVisibility || !this->Histogram || upToDate)
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

  int imageWidth = this->DisplaySize[0] - 2*this->BorderWidth;
  int imageHeight = this->DisplaySize[1] - 2*this->BorderWidth;
  double range[2];
  this->Histogram->GetRange(range);
  double logRange = log(range[1]);
  unsigned char color[3];
  color[0] = static_cast<unsigned char>(this->HistogramColor[0] * 255);
  color[1] = static_cast<unsigned char>(this->HistogramColor[1] * 255);
  color[2] = static_cast<unsigned char>(this->HistogramColor[2] * 255);
  double scalarInc =
    (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]) /
    (double)(imageWidth);
  double scalar = this->VisibleScalarRange[0];
  double dColor[3];

  int i, j, histogramIdx, height;

  for (i = 0; i < imageWidth; i++, scalar += scalarInc)
    {
    histogramIdx = vtkMath::Floor(i * numBins / imageWidth);
    histogramIdx += minBinIdx;
    if (histogramIdx < 0 || histogramIdx > maxBinIdx)
      {
      height = 0;
      }
    else
      {
      height = vtkMath::Floor(
        log((double)(this->Histogram->GetValue(histogramIdx))) *
        imageHeight / logRange);
      }

    if (height && this->ShowColorFunctionInHistogram && this->ColorFunction)
      {
      this->ColorFunction->GetColor(scalar, dColor);
      color[0] = static_cast<unsigned char>(255 * dColor[0]);
      color[1] = static_cast<unsigned char>(255 * dColor[1]);
      color[2] = static_cast<unsigned char>(255 * dColor[2]);
      }

    for (j = 0; j < height; j++)
      {
      scalars->SetComponent(j * imageWidth + i, 0, color[0]);
      scalars->SetComponent(j * imageWidth + i, 1, color[1]);
      scalars->SetComponent(j * imageWidth + i, 2, color[2]);
      scalars->SetComponent(j * imageWidth + i, 3, 255);
      }
    for (j = height; j < imageHeight; j++)
      {
      scalars->SetComponent(j * imageWidth + i, 0, 0);
      scalars->SetComponent(j * imageWidth + i, 1, 0);
      scalars->SetComponent(j * imageWidth + i, 2, 0);
      scalars->SetComponent(j * imageWidth + i, 3, 0);
      }
    }

  this->HistogramImage->Modified();
  this->UpdateHistogramMTime();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::BuildRepresentation()
{
  if (this->HistogramVisibility)
    {
    this->UpdateHistogramImage();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::SetColorFunction(
  vtkColorTransferFunction *color)
{
  this->Superclass::SetColorFunction(color);
  this->BackgroundMapper->SetLookupTable(color);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation1D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
