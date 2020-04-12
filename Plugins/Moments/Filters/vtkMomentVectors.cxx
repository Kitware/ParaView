// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentVectors.cxx

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

#include "vtkMomentVectors.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
// Computes the direction of a 1D cell.
inline void vtkMomentVectorsCellDirection(vtkCell* cell, double vec[3])
{
  double p0[3], p1[3];
  vtkPoints* points = cell->GetPoints();
  points->GetPoint(0, p0);
  points->GetPoint(cell->GetNumberOfPoints() - 1, p1);
  for (int j = 0; j < 3; j++)
    vec[j] = p1[j] - p0[j];
  vtkMath::Normalize(vec);
}

// Computes the normal of a 2D cell.
inline void vtkMomentVectorsCellNormal(vtkCell* cell, double vec[3])
{
  double p0[3], p1[3], p2[3], v0[3], v1[3];
  vtkPoints* points = cell->GetPoints();
  points->GetPoint(0, p0);
  points->GetPoint(1, p1);
  points->GetPoint(2, p2);
  int j;
  for (j = 0; j < 3; j++)
    v0[j] = p0[j] - p1[j];
  for (j = 0; j < 3; j++)
    v1[j] = p2[j] - p1[j];
  vtkMath::Cross(v1, v0, vec);
  vtkMath::Normalize(vec);
}

//-----------------------------------------------------------------------------
// Computes the length of a 1D cell (only accurate for 1 segment cells).
inline double vtkMomentVectorsCellLength(vtkCell* cell)
{
  vtkPoints* points = cell->GetPoints();
  double p0[3], p1[3];
  points->GetPoint(0, p0);
  points->GetPoint(cell->GetNumberOfPoints() - 1, p1);
  return sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
}

// Computes the area of a 2D cell.
inline double vtkMomentVectorsCellArea(vtkCell* cell)
{
  VTK_CREATE(vtkIdList, triangleIds);
  VTK_CREATE(vtkPoints, points);
  cell->Triangulate(0, triangleIds, points);
  int numTris = points->GetNumberOfPoints() / 3;

  double totalArea = 0.0;
  for (int i = 0; i < numTris; i++)
  {
    double p0[3], p1[3], p2[3], v0[3], v1[3], vec[3];
    int j;
    points->GetPoint(i * 3 + 0, p0);
    points->GetPoint(i * 3 + 1, p1);
    points->GetPoint(i * 3 + 2, p2);
    for (j = 0; j < 3; j++)
      v0[j] = p0[j] - p1[j];
    for (j = 0; j < 3; j++)
      v1[j] = p2[j] - p1[j];
    // The magnitude of the cross product is the area of the parallelogram of
    // the two vectors.  Half that is the area of the triangle.
    vtkMath::Cross(v1, v0, vec);
    totalArea += 0.5 * vtkMath::Norm(vec);
  }

  return totalArea;
}

//=============================================================================
vtkStandardNewMacro(vtkMomentVectors);

//-----------------------------------------------------------------------------
vtkMomentVectors::vtkMomentVectors()
{
  this->SetInputMoment(vtkDataSetAttributes::SCALARS);
  this->InputMomentIsDensity = 0;
  this->OutputMomentTotalName = NULL;
  this->OutputMomentDensityName = NULL;
}

vtkMomentVectors::~vtkMomentVectors()
{
}

void vtkMomentVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputMomentIsDensity: " << this->InputMomentIsDensity << endl;
  os << indent << "OutputMomentTotalName: " << this->GetOutputMomentTotalName() << endl;
  os << indent << "OutputMomentDensityName: " << this->GetOutputMomentDensityName() << endl;
}

//-----------------------------------------------------------------------------
void vtkMomentVectors::SetInputMoment(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, name);
}

void vtkMomentVectors::SetInputMoment(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldAttributeType);
}

//-----------------------------------------------------------------------------
const char* vtkMomentVectors::GetOutputMomentTotalName(vtkDataObject* input)
{
  if (this->OutputMomentTotalName && (this->OutputMomentTotalName[0] != '\0'))
  {
    return this->OutputMomentTotalName;
  }

  if (!input)
    return "???";

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, input);
  if (!inputArray)
    return "???";

  if (this->InputMomentIsDensity)
  {
    static std::string result;
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
const char* vtkMomentVectors::GetOutputMomentDensityName(vtkDataObject* input)
{
  if (this->OutputMomentDensityName && (this->OutputMomentDensityName[0] != '\0'))
  {
    return this->OutputMomentDensityName;
  }

  if (!input)
    return "???";

  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, input);
  if (!inputArray)
    return "???";

  if (this->InputMomentIsDensity)
  {
    return inputArray->GetName();
  }
  else
  {
    static std::string result;
    result = inputArray->GetName();
    result += "_density";
    return result.c_str();
  }
}

//-----------------------------------------------------------------------------
int vtkMomentVectors::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro(<< "Missing input or output?");
    return 0;
  }

  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  vtkDataArray* scalars = this->GetInputArrayToProcess(0, inputVector);

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
  fluxTotalOut->SetName(this->GetOutputMomentTotalName(input));
  fluxTotalOut->SetNumberOfComponents(3);
  fluxTotalOut->SetNumberOfTuples(numCells);

  VTK_CREATE(vtkDoubleArray, fluxDensityOut);
  fluxDensityOut->SetName(this->GetOutputMomentDensityName(input));
  fluxDensityOut->SetNumberOfComponents(3);
  fluxDensityOut->SetNumberOfTuples(numCells);

  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
  {
    input->GetCell(cellId, cell);

    double s = scalars->GetTuple1(cellId); // The input scalar.
    double vec[3];                         // The output vector.
    double size;                           // Cell size (length or area)

    int j;

    switch (cell->GetCellDimension())
    {
      case 1:
        vtkMomentVectorsCellDirection(cell, vec);
        size = vtkMomentVectorsCellLength(cell);
        break;
      case 2:
        vtkMomentVectorsCellNormal(cell, vec);
        size = vtkMomentVectorsCellArea(cell);
        break;
      default:
        // Invalid cell type.  Should we warn?
        for (j = 0; j < 3; j++)
          vec[j] = 0.0;
        size = 1.0;
        break;
    }

    for (j = 0; j < 3; j++)
      vec[j] *= s;
    if (this->InputMomentIsDensity)
    {
      fluxDensityOut->SetTuple(cellId, vec);
      for (j = 0; j < 3; j++)
        vec[j] *= size;
      fluxTotalOut->SetTuple(cellId, vec);
    }
    else
    {
      fluxTotalOut->SetTuple(cellId, vec);
      for (j = 0; j < 3; j++)
        vec[j] /= size;
      fluxDensityOut->SetTuple(cellId, vec);
    }
  }

  output->GetCellData()->AddArray(fluxTotalOut);
  output->GetCellData()->AddArray(fluxDensityOut);
  if (input->GetCellData()->GetScalars() == scalars)
  {
    if (this->InputMomentIsDensity)
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
