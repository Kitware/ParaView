// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFluxVectors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkFluxVectors.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//=============================================================================
vtkCxxRevisionMacro(vtkFluxVectors, "1.1");
vtkStandardNewMacro(vtkFluxVectors);

//-----------------------------------------------------------------------------
vtkFluxVectors::vtkFluxVectors()
{
  this->SetInputArray(vtkDataSetAttributes::SCALARS);
}

vtkFluxVectors::~vtkFluxVectors()
{
}

void vtkFluxVectors::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkFluxVectors::SetInputArray(const char *name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               name);
}

void vtkFluxVectors::SetInputArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               fieldAttributeType);
}

//-----------------------------------------------------------------------------
int vtkFluxVectors::RequestData(vtkInformation *vtkNotUsed(request),
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet *output = vtkDataSet::GetData(outputVector);

  if (!input || !output)
    {
    vtkErrorMacro(<< "Missing input or output?");
    return 0;
    }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkDataArray *scalars = this->GetInputArrayToProcess(0, inputVector);

  if (scalars == NULL)
    {
    vtkDebugMacro("No input scalars.");
    return 1;
    }
  if (scalars->GetNumberOfComponents() != 1)
    {
    vtkErrorMacro("Input array must have one component.");
    return 0;
    }
  if (scalars->GetName() == NULL)
    {
    vtkErrorMacro("Input array needs a name.");
    return 0;
    }

  output->GetCellData()->RemoveArray(scalars->GetName());

  vtkIdType numCells = input->GetNumberOfCells();
  VTK_CREATE(vtkGenericCell, cell);
  VTK_CREATE(vtkDoubleArray, vectors);
  vectors->SetName(scalars->GetName());
  vectors->SetNumberOfComponents(3);
  vectors->SetNumberOfTuples(numCells);

  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId, cell);

    vtkPoints *points = cell->GetPoints(); // Points associated with the cell.

    double s = scalars->GetTuple1(cellId);              // The input scalar.
    double *vec = vectors->WritePointer(cellId*3, 3);   // The output vector

    double p0[3], p1[3], p2[3], v0[3], v1[3];   //Scratch variables.

    int j;

    switch (cell->GetCellDimension())
      {
      case 1:
        points->GetPoint(0, p0);
        points->GetPoint(cell->GetNumberOfPoints()-1, p1);
        for (j = 0; j < 3; j++) vec[j] = s*(p1[j] - p0[j]);
        break;
      case 2:
        points->GetPoint(0, p0);
        points->GetPoint(1, p1);
        points->GetPoint(2, p2);
        for (j = 0; j < 3; j++) v0[j] = p0[j] - p1[j];
        for (j = 0; j < 3; j++) v1[j] = p2[j] - p1[j];
        vtkMath::Cross(v1, v0, vec);
        vtkMath::Normalize(vec);
        for (j = 0; j < 3; j++) vec[j] *= s;
        break;
      default:
        // Invalid cell type.  Should we warn?
        for (j = 0; j < 3; j++) vec[j] = 0.0;
        break;
      }
    }

  output->GetCellData()->AddArray(vectors);
  if (input->GetCellData()->GetScalars() == scalars)
    {
    output->GetCellData()->SetVectors(vectors);
    }

  return 1;
}

