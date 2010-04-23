/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidget1D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidget1D.h"

#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkRectilinearGrid.h"
#include "vtkTransferFunctionEditorRepresentation1D.h"


//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget1D::vtkTransferFunctionEditorWidget1D()
{
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget1D::~vtkTransferFunctionEditorWidget1D()
{
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::SetHistogram(
  vtkRectilinearGrid *histogram)
{
  this->Superclass::SetHistogram(histogram);

  if (histogram)
    {
    vtkTransferFunctionEditorRepresentation1D *rep =
      vtkTransferFunctionEditorRepresentation1D::SafeDownCast(this->WidgetRep);
    if (rep)
      {
      vtkIntArray *histValues = vtkIntArray::SafeDownCast(
        histogram->GetCellData()->GetArray("bin_values"));
      if (histValues)
        {
        rep->SetHistogram(histValues);
        }
      else
        {
        vtkErrorMacro("Histogram does not have cell-centered array called bin_values.")
        }
      }
    }
}

//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidget1D::ComputeScalar(double pos, int width)
{
  double pct =
    (pos - this->BorderWidth) / (double)(width - 2*this->BorderWidth);
  return this->VisibleScalarRange[0] + pct *
    (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
}

//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidget1D::ComputePositionFromScalar(
  double scalar, int width)
{
  double pct = (scalar - this->VisibleScalarRange[0]);
  if (this->VisibleScalarRange[0] != this->VisibleScalarRange[1])
    {
    pct /= this->VisibleScalarRange[1] - this->VisibleScalarRange[0];
    }
  return (width - 2*this->BorderWidth) * pct + this->BorderWidth;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
