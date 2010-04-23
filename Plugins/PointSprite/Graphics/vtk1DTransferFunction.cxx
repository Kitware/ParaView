/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunction
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

#include "vtk1DTransferFunction.h"

#include "vtkObjectFactory.h"
#include "vtkDataArray.h"


vtk1DTransferFunction::vtk1DTransferFunction()
{
  this->InputRange[0] = 0.0;
  this->InputRange[1] = 1.0;
  this->UseScalarRange = 1;
  this->VectorComponent = -1;
}

vtk1DTransferFunction::~vtk1DTransferFunction()
{
}

void vtk1DTransferFunction::MapArray(vtkDataArray* input, vtkDataArray* output)
{
  double range[2];
  if (this->UseScalarRange)
    {
    input->GetRange(range, this->VectorComponent);
    }
  else
    {
    range[0] = this->InputRange[0];
    range[1] = this->InputRange[1];
    }
  output->SetNumberOfComponents(1);
  output->SetNumberOfTuples(input->GetNumberOfTuples());

  double mappedValue;
  vtkIdType index;
  for (index = 0; index < input->GetNumberOfTuples(); index++)
    {
    double val;
    int comp = this->VectorComponent;
    if(comp == -1 && input->GetNumberOfComponents() == 1)
      {
      comp = 0;
      }
    if (comp == -1 )
      {
      double norm2 = 0;
      double *tuple = input->GetTuple(index);
      for (int i = 0; i < input->GetNumberOfComponents(); i++)
        {
        norm2 += tuple[i] * tuple[i];
        }
      val = sqrt(norm2);
      }
    else
      {
      val = input->GetTuple(index)[comp];
      }
    mappedValue = this->MapValue(val, range);
    output->SetTuple1(index, mappedValue);
    }
}

void vtk1DTransferFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputRange : " << InputRange[0] << " " << InputRange[1]
      << endl;
}

