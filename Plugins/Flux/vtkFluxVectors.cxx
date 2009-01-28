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
#include "vtkStdString.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
// Computes the direction of a 1D cell.
inline void vtkFluxVectorsCellDirection(vtkCell *cell, double vec[3])
{
  double p0[3], p1[3];
  vtkPoints *points = cell->GetPoints();
  points->GetPoint(0, p0);
  points->GetPoint(cell->GetNumberOfPoints()-1, p1);
  for (int j = 0; j < 3; j++) vec[j] = p1[j] - p0[j];
}

// Computes the normal of a 2D cell.
inline void vtkFluxVectorsCellNormal(vtkCell *cell, double vec[3])
{
  double p0[3], p1[3], p2[3], v0[3], v1[3];
  vtkPoints *points = cell->GetPoints();
  points->GetPoint(0, p0);
  points->GetPoint(1, p1);
  points->GetPoint(2, p2);
  int j;
  for (j = 0; j < 3; j++) v0[j] = p0[j] - p1[j];
  for (j = 0; j < 3; j++) v1[j] = p2[j] - p1[j];
  vtkMath::Cross(v1, v0, vec);
  vtkMath::Normalize(vec);
 }

//-----------------------------------------------------------------------------
// Computes the length of a 1D cell (only accurate for 1 segment cells).
inline double vtkFluxVectorsCellLength(vtkCell *cell)
{
  vtkPoints *points = cell->GetPoints();
  double p0[3], p1[3];
  points->GetPoint(0, p0);
  points->GetPoint(cell->GetNumberOfPoints()-1, p1);
  return sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
}

// Computes the area of a 2D cell.
inline double vtkFluxVectorsCellArea(vtkCell *cell)
{
  VTK_CREATE(vtkIdList, triangleIds);
  VTK_CREATE(vtkPoints, points);
  cell->Triangulate(0, triangleIds, points);
  int numTris = points->GetNumberOfPoints()/3;

  double totalArea = 0.0;
  for (int i = 0; i < numTris; i++)
    {
    double p0[3], p1[3], p2[3], v0[3], v1[3], vec[3];
    int j;
    points->GetPoint(i*3+0, p0);
    points->GetPoint(i*3+1, p1);
    points->GetPoint(i*3+2, p2);
    for (j = 0; j < 3; j++) v0[j] = p0[j] - p1[j];
    for (j = 0; j < 3; j++) v1[j] = p2[j] - p1[j];
    // The magnitude of the cross product is the area of the parallelogram of
    // the two vectors.  Half that is the area of the triangle.
    vtkMath::Cross(v1, v0, vec);
    totalArea += 0.5*vtkMath::Norm(vec);
    }

  return totalArea;
}

//=============================================================================
vtkCxxRevisionMacro(vtkFluxVectors, "1.2");
vtkStandardNewMacro(vtkFluxVectors);

//-----------------------------------------------------------------------------
vtkFluxVectors::vtkFluxVectors()
{
  this->SetInputFlux(vtkDataSetAttributes::SCALARS);
  this->InputFluxIsDensity = 0;
  this->OutputFluxTotalName = NULL;
  this->OutputFluxDensityName = NULL;
}

vtkFluxVectors::~vtkFluxVectors()
{
}

void vtkFluxVectors::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputFluxIsDensity: " << this->InputFluxIsDensity << endl;
  os << indent << "OutputFluxTotalName: "
     << this->GetOutputFluxTotalName() << endl;
  os << indent << "OutputFluxDensityName: "
     << this->GetOutputFluxDensityName() << endl;
}

//-----------------------------------------------------------------------------
void vtkFluxVectors::SetInputFlux(const char *name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               name);
}

void vtkFluxVectors::SetInputFlux(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               fieldAttributeType);
}

//-----------------------------------------------------------------------------
const char *vtkFluxVectors::GetOutputFluxTotalName(vtkDataObject *input)
{
  if (this->OutputFluxTotalName && (this->OutputFluxTotalName[0] != '\0'))
    {
    return this->OutputFluxTotalName;
    }

  if (!input) return "???";

  vtkDataArray *inputArray = this->GetInputArrayToProcess(0, input);
  if (!inputArray) return "???";

  if (this->InputFluxIsDensity)
    {
    static vtkStdString result;
    result = inputArray->GetName();
    result += "_total";
    return result.c_str();
    }
  else
    {
    return inputArray->GetName();
    }
}

//-----------------------------------------------------------------------------
const char *vtkFluxVectors::GetOutputFluxDensityName(vtkDataObject *input)
{
  if (this->OutputFluxDensityName && (this->OutputFluxDensityName[0] != '\0'))
    {
    return this->OutputFluxDensityName;
    }

  if (!input) return "???";

  vtkDataArray *inputArray = this->GetInputArrayToProcess(0, input);
  if (!inputArray) return "???";

  if (this->InputFluxIsDensity)
    {
    return inputArray->GetName();
    }
  else
    {
    static vtkStdString result;
    result = inputArray->GetName();
    result += "_density";
    return result.c_str();
    }
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

  VTK_CREATE(vtkDoubleArray, fluxTotalOut);
  fluxTotalOut->SetName(this->GetOutputFluxTotalName(input));
  fluxTotalOut->SetNumberOfComponents(3);
  fluxTotalOut->SetNumberOfTuples(numCells);

  VTK_CREATE(vtkDoubleArray, fluxDensityOut);
  fluxDensityOut->SetName(this->GetOutputFluxDensityName(input));
  fluxDensityOut->SetNumberOfComponents(3);
  fluxDensityOut->SetNumberOfTuples(numCells);

  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId, cell);

    double s = scalars->GetTuple1(cellId);      // The input scalar.
    double vec[3];                              // The output vector.
    double size;                                // Cell size (length or area)

    int j;

    switch (cell->GetCellDimension())
      {
      case 1:
        vtkFluxVectorsCellDirection(cell, vec);
        size = vtkFluxVectorsCellLength(cell);
        break;
      case 2:
        vtkFluxVectorsCellNormal(cell, vec);
        size = vtkFluxVectorsCellArea(cell);
        break;
      default:
        // Invalid cell type.  Should we warn?
        for (j = 0; j < 3; j++) vec[j] = 0.0;
        size = 1.0;
        break;
      }

    for (j = 0; j < 3; j++) vec[j] *= s;
    if (this->InputFluxIsDensity)
      {
      fluxDensityOut->SetTuple(cellId, vec);
      for (j = 0; j < 3; j++) vec[j] *= size;
      fluxTotalOut->SetTuple(cellId, vec);
      }
    else
      {
      fluxTotalOut->SetTuple(cellId, vec);
      for (j = 0; j < 3; j++) vec[j] /= size;
      fluxDensityOut->SetTuple(cellId, vec);
      }
    }

  output->GetCellData()->AddArray(fluxTotalOut);
  output->GetCellData()->AddArray(fluxDensityOut);
  if (input->GetCellData()->GetScalars() == scalars)
    {
    if (this->InputFluxIsDensity)
      {
      output->GetCellData()->SetVectors(fluxDensityOut);
      }
    else
      {
      output->GetCellData()->SetVectors(fluxTotalOut);
      }
    }

  return 1;
}

