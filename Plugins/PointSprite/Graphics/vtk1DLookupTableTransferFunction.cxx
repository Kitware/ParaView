/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DLookupTableTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DLookupTableTransferFunction
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

#include "vtk1DLookupTableTransferFunction.h"

#include "vtkObjectFactory.h"
#include "vtkDoubleArray.h"

#include <cmath>

vtkStandardNewMacro(vtk1DLookupTableTransferFunction)

vtk1DLookupTableTransferFunction::vtk1DLookupTableTransferFunction()
{
  this->Table = vtkDoubleArray::New();
  this->Interpolation = 0;
}

vtk1DLookupTableTransferFunction::~vtk1DLookupTableTransferFunction()
{
  this->Table->Delete();
}

double vtk1DLookupTableTransferFunction::MapValue(double value, double* range)
{
  double diff = range[1] - range[0];

  double output = 0;
  if (diff == 0)
    {
    vtkDebugMacro("input range min and max do match!");
    if (value < range[0])
      output = this->Table->GetTuple1(0);
    else
      output = this->Table->GetTuple1(this->Table->GetNumberOfTuples() - 1);
    }
  else
    {
    double ratio = (value - range[0]) / diff * this->Table->GetNumberOfTuples();
    if (ratio <= 0)
      {
      output = this->Table->GetTuple1(0);
      }
    else if (ratio >= this->Table->GetNumberOfTuples())
      {
      output = this->Table->GetTuple1(this->Table->GetNumberOfTuples() - 1);
      }
    else
      {
      vtkIdType index = static_cast<vtkIdType> (floor(ratio));
      double first = this->Table->GetTuple1(index);
      if (Interpolation)
        {
        double delta = ratio - index;
        double second;
        if (index < this->Table->GetNumberOfTuples() - 1)
          {
          second = this->Table->GetTuple1(index + 1);
          }
        else
          {
          second = this->Table->GetTuple1(index);
          }
        output = first * (1.0 - delta) + delta * second;
        }
      else // no interpolation
        {
        output = first;
        }
      }
    }
  return output;
}

void vtk1DLookupTableTransferFunction::BuildDefaultTable()
{
  const vtkIdType ntuples = 256;
  this->Table->SetNumberOfComponents(1);
  this->Table->SetNumberOfTuples(ntuples);
  this->Table->Allocate(ntuples);
  for (vtkIdType i = 0; i < ntuples; i++)
    {
    this->Table->SetTuple1(i, i / double(ntuples - 1));
    }
}

void  vtk1DLookupTableTransferFunction::SetNumberOfTableValues(vtkIdType size)
{
  if(this->Table->GetNumberOfTuples() != size)
    {
    this->Table->SetNumberOfTuples(size);
    this->Modified();
    }
}

vtkIdType  vtk1DLookupTableTransferFunction::GetNumberOfTableValues()
{
  return this->Table->GetNumberOfTuples();
}

void  vtk1DLookupTableTransferFunction::SetTableValue(vtkIdType index, double value)
{
  if(index < 0)
    return;
  bool modif = false;
  if(index >= this->Table->GetNumberOfTuples())
    {
    this->Table->SetNumberOfTuples(index+1);
    modif = true;
    }
  if(this->Table->GetTuple1(index) != value)
    {
    this->Table->SetValue(index, value);
    modif = true;
    }
  if(modif)
    this->Modified();
}

void  vtk1DLookupTableTransferFunction::RemoveAllTableValues()
{
  this->SetNumberOfTableValues(0);
}


double  vtk1DLookupTableTransferFunction::GetTableValue(vtkIdType index)
{
  if(index < 0 || index >= this->Table->GetNumberOfTuples())
    {
    vtkWarningMacro("Trying to get out of range table value, returning 0.");
    return 0;
    }
  return this->Table->GetTuple1(index);
}

void vtk1DLookupTableTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputRange : " << InputRange[0] << " " << InputRange[1]
      << endl;
  this->Table->PrintSelf(os, indent.GetNextIndent());
}

