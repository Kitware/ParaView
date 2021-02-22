/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceVectors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceVectors.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

vtkStandardNewMacro(vtkSurfaceVectors);

//-----------------------------------------------------------------------------
// Construct with feature angle=30, splitting and consistency turned on,
// flipNormals turned off, and non-manifold traversal turned on.
vtkSurfaceVectors::vtkSurfaceVectors()
{
  this->ConstraintMode = vtkSurfaceVectors::Parallel;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
vtkSurfaceVectors::~vtkSurfaceVectors() = default;

//-----------------------------------------------------------------------------
int vtkSurfaceVectors::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()) + 1);

  return 1;
}

//-----------------------------------------------------------------------------
// Generate normals for polygon meshesPrint
//----------------------------------------------------------------------------
int vtkSurfaceVectors::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  const vtkIdType numPoints = input->GetNumberOfPoints();
  vtkDataArray* inVectors = this->GetInputArrayToProcess(0, inputVector);

  if (!inVectors || numPoints == 0)
  {
    vtkErrorMacro("The input is empty");
    output->ShallowCopy(input);
    return 1;
  }

  vtkDataArray* newVectors = nullptr;
  vtkDoubleArray* newScalars = nullptr;
  vtkIdList* cellIds = vtkIdList::New();
  vtkIdList* ptIds = vtkIdList::New();

  if (this->ConstraintMode == vtkSurfaceVectors::PerpendicularScale)
  {
    newScalars = vtkDoubleArray::New();
    newScalars->SetNumberOfComponents(1);
    newScalars->SetNumberOfTuples(numPoints);
    newScalars->SetName("Perpendicular Scale");
  }
  else
  {
    newVectors = inVectors->NewInstance();
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(numPoints);
    newVectors->SetName(inVectors->GetName());
  }

  // Helper function to compute the normal of a point
  const auto ComputeNormal = [&input, &cellIds, &ptIds]() -> vtkVector3d {
    vtkVector3d normal(0.0);
    for (int i = 0; i < cellIds->GetNumberOfIds(); ++i)
    {
      const vtkIdType cellId = cellIds->GetId(i);
      const vtkIdType cellType = input->GetCellType(cellId);

      if (cellType == VTK_VOXEL || cellType == VTK_POLYGON || cellType == VTK_TRIANGLE ||
        cellType == VTK_QUAD || cellType == VTK_PIXEL)
      {
        input->GetCellPoints(cellId, ptIds);

        vtkVector3d p1, p2, p3;
        input->GetPoint(ptIds->GetId(0), p1.GetData());
        input->GetPoint(ptIds->GetId(1), p2.GetData());
        input->GetPoint(ptIds->GetId(2), p3.GetData());

        const vtkVector3d v1 = p2 - p1;
        const vtkVector3d v2 = p3 - p1;

        const vtkVector3d cross = v1.Cross(v2);

        // We check the current normal orientation against
        // the computed one so far: if they have the same orientation
        // (ie the scalar product is positive) we add it to normal
        // otherwise we add its negated version.
        //
        // This ensures that two opposite normals don't cancel each other
        // (for example (1, 0, 0) and (-1, 0, 0) gives the same general
        // direction but the sum is zero).
        normal += cross.Dot(normal) > 0 ? cross : -cross;
      }
    }

    return normal.Normalized();
  };

  for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
  {
    input->GetPointCells(pointId, cellIds);

    const vtkVector3d normal = ComputeNormal();

    vtkVector3d inVector;
    inVectors->GetTuple(pointId, inVector.GetData());
    const double k = normal.Dot(inVector);

    switch (this->ConstraintMode)
    {
      case vtkSurfaceVectors::Parallel:
        inVector -= k * normal;
        newVectors->InsertTuple(pointId, inVector.GetData());
        break;
      case vtkSurfaceVectors::Perpendicular:
        inVector = k * normal;
        newVectors->InsertTuple(pointId, inVector.GetData());
        break;
      default:
        newScalars->InsertValue(pointId, k);
        break;
    }
  }

  output->ShallowCopy(input);
  if (newVectors)
  {
    output->GetPointData()->SetVectors(newVectors);
    newVectors->Delete();
  }
  if (newScalars)
  {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
  }
  cellIds->Delete();
  ptIds->Delete();

  // Not implemented for data set.
  // output->RemoveGhostCells
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSurfaceVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ConstraintMode == vtkSurfaceVectors::Parallel)
  {
    os << indent << "ConstraintMode: Parallel\n";
  }
  else if (this->ConstraintMode == vtkSurfaceVectors::Perpendicular)
  {
    os << indent << "ConstraintMode: Perpendicular\n";
  }
  else if (this->ConstraintMode == vtkSurfaceVectors::PerpendicularScale)
  {
    os << indent << "ConstraintMode: PerpendicularScale\n";
  }
  else
  {
    os << indent << "ConstraintMode: Unknown\n";
  }
}
