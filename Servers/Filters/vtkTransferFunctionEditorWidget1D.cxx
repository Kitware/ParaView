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

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidget1D, "1.4");

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
        histogram->GetCellData()->GetArray("bin values"));
      if (histValues)
        {
        rep->SetHistogram(histValues);
        }
      else
        {
        vtkErrorMacro("Histogram does not have cell-centered array called bin values.")
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget1D::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
