/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRayCastPickingHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRayCastPickingHelper - helper class that used selection and ray
// casting to find the intersection point between the user picking point
// and the concrete cell underneath.
#include "vtkPVRayCastPickingHelper.h"

#include "vtkCell.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtractSelection.h"
#include "vtkPointData.h"
#include "vtkPolygon.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangle.h"

#include <limits>

vtkStandardNewMacro(vtkPVRayCastPickingHelper);
vtkCxxSetObjectMacro(vtkPVRayCastPickingHelper, Input, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkPVRayCastPickingHelper, Selection, vtkAlgorithm);

//----------------------------------------------------------------------------
vtkPVRayCastPickingHelper::vtkPVRayCastPickingHelper()
{
  this->Selection = nullptr;
  this->Input = nullptr;
  this->SnapOnMeshPoint = false;
  this->PointA[0] = this->PointA[1] = this->PointA[2] = 0.0;
  this->PointB[0] = this->PointB[1] = this->PointB[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkPVRayCastPickingHelper::~vtkPVRayCastPickingHelper()
{
  this->SetSelection(nullptr);
  this->SetInput(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVRayCastPickingHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "PointA: " << this->PointA[0] << ", " << this->PointA[1] << ", "
     << this->PointA[2] << endl;
  os << indent << "PointB: " << this->PointB[0] << ", " << this->PointB[1] << ", "
     << this->PointB[2] << endl;
  os << indent << "SnapOnMeshPoint: " << this->SnapOnMeshPoint << endl;
  os << indent << "Last Intersection: " << this->Intersection[0] << ", " << this->Intersection[1]
     << ", " << this->Intersection[2] << endl;
  os << indent << "Last Intersection Normal: " << this->IntersectionNormal[0] << ", "
     << this->IntersectionNormal[1] << ", " << this->IntersectionNormal[2] << endl;
  os << indent << "Input: " << (this->Input ? this->Input->GetClassName() : "NULL") << endl;
  os << indent << "Selection: " << (this->Selection ? this->Selection->GetClassName() : "NULL")
     << endl;
}

//----------------------------------------------------------------------------
void vtkPVRayCastPickingHelper::ComputeIntersection()
{
  // Need valid input.
  if (!this->Input || !this->Selection)
  {
    return;
  }
  // Need valid ray.
  if (!vtkMath::Distance2BetweenPoints(this->PointA, this->PointB))
  {
    return;
  }

  // Reset the Intersection and IntersectionNormal values
  this->Intersection[0] = this->Intersection[1] = this->Intersection[2] = 0.0;
  this->IntersectionNormal[0] = this->IntersectionNormal[1] = this->IntersectionNormal[2] = 0.0;

  // Manage multi-process distribution
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int pid = controller->GetLocalProcessId();
  int numberOfProcesses = controller->GetNumberOfProcesses();

  vtkNew<vtkPVExtractSelection> extractSelectionFilter;
  extractSelectionFilter->SetInputConnection(0, this->Input->GetOutputPort(0));
  extractSelectionFilter->SetInputConnection(1, this->Selection->GetOutputPort(0));
  extractSelectionFilter->UpdatePiece(pid, numberOfProcesses, 0);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(extractSelectionFilter->GetOutput());
  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(extractSelectionFilter->GetOutput());
  if (ds && ds->GetNumberOfCells() > 0)
  {
    this->ComputeIntersectionFromDataSet(ds);
  }
  else if (cds)
  {
    vtkSmartPointer<vtkCompositeDataIterator> dsIter;
    dsIter.TakeReference(cds->NewIterator());
    for (dsIter->GoToFirstItem(); !dsIter->IsDoneWithTraversal(); dsIter->GoToNextItem())
    {
      this->ComputeIntersectionFromDataSet(
        vtkDataSet::SafeDownCast(dsIter->GetCurrentDataObject()));
    }
  }

  // If distributed do a global reduction and make sure the root node get the
  // right value. We don't care about the other nodes...
  if (numberOfProcesses > 1)
  {
    double result[3];
    controller->Reduce(this->Intersection, result, 3, vtkCommunicator::SUM_OP, 0);
    this->Intersection[0] = result[0];
    this->Intersection[1] = result[1];
    this->Intersection[2] = result[2];

    controller->Reduce(this->IntersectionNormal, result, 3, vtkCommunicator::SUM_OP, 0);
    this->IntersectionNormal[0] = result[0];
    this->IntersectionNormal[1] = result[1];
    this->IntersectionNormal[2] = result[2];
  }
}

//----------------------------------------------------------------------------
static const double IntersectionTolerance = 0.0000000001;

//----------------------------------------------------------------------------
int vtkPVRayCastPickingHelper::ComputeSurfaceNormal(
  vtkDataSet* data, vtkCell* cell, int subId, double* weights)
{
  vtkDataArray* normals = data->GetPointData()->GetNormals();

  if (normals)
  {
    this->IntersectionNormal[0] = this->IntersectionNormal[1] = this->IntersectionNormal[2] = 0.0;
    double pointNormal[3];
    const vtkIdType numPoints = cell->GetNumberOfPoints();
    for (vtkIdType k = 0; k < numPoints; k++)
    {
      normals->GetTuple(cell->PointIds->GetId(k), pointNormal);
      this->IntersectionNormal[0] += pointNormal[0] * weights[k];
      this->IntersectionNormal[1] += pointNormal[1] * weights[k];
      this->IntersectionNormal[2] += pointNormal[2] * weights[k];
    }
    vtkMath::Normalize(this->IntersectionNormal);
  }
  else
  {
    if (cell->GetCellDimension() == 3)
    {
      double t;
      int faceSubId;
      double pcoord[3], x[3];

      int closestIntersectedFaceId = -1;
      double minDist2 = VTK_DOUBLE_MAX;
      // find the face that the ray intersected with that is closer to the intersection point
      for (int i = 0; i < cell->GetNumberOfFaces(); ++i)
      {
        if (cell->GetFace(i)->IntersectWithLine(
              this->PointA, this->PointB, IntersectionTolerance, t, x, pcoord, faceSubId) != 0 &&
          t != VTK_DOUBLE_MAX)
        {
          double dist2 = vtkMath::Distance2BetweenPoints(x, this->Intersection);
          if (dist2 < minDist2)
          {
            minDist2 = dist2;
            closestIntersectedFaceId = i;
          }
        }
      }
      if (closestIntersectedFaceId != -1)
      {
        // calculate the normal of the 2D face
        vtkPolygon::ComputeNormal(
          cell->GetFace(closestIntersectedFaceId)->Points, this->IntersectionNormal);
      }
      else
      {
        return 0;
      }
    }
    else if (cell->GetCellDimension() == 2)
    {
      if (cell->GetCellType() != VTK_TRIANGLE_STRIP)
      {
        // calculate the normal of the 2D cell
        vtkPolygon::ComputeNormal(cell->Points, this->IntersectionNormal);
      }
      else // cell->GetCellType() == VTK_TRIANGLE_STRIP
      {
        static int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
        int* order = idx[subId & 1];
        vtkIdType pointIds[3];
        double points[3][3];

        pointIds[0] = cell->PointIds->GetId(subId + order[0]);
        pointIds[1] = cell->PointIds->GetId(subId + order[1]);
        pointIds[2] = cell->PointIds->GetId(subId + order[2]);

        data->GetPoint(pointIds[0], points[0]);
        data->GetPoint(pointIds[1], points[1]);
        data->GetPoint(pointIds[2], points[2]);

        // calculate the normal of the subId triangle of the triangle strip cell
        vtkTriangle::ComputeNormal(points[0], points[1], points[2], this->IntersectionNormal);
      }
    }
    else
    {
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVRayCastPickingHelper::ComputeIntersectionFromDataSet(vtkDataSet* ds)
{
  if (ds && ds->GetNumberOfCells() > 0)
  {
    if (this->SnapOnMeshPoint) // if we are snapping
    {
      ds->GetPoint(0, this->Intersection);
      vtkDataArray* normals = ds->GetPointData()->GetNormals();
      if (normals != nullptr) // if point normals exist
      {
        normals->GetTuple(0, this->IntersectionNormal);
      }
      else
      {
        this->IntersectionNormal[0] = this->IntersectionNormal[1] = this->IntersectionNormal[2] =
          std::numeric_limits<double>::quiet_NaN();
      }
    }
    else // if we are not snapping
    {
      double t;
      int subId;
      double pcoord[3], x[3];
      vtkCell* cell = ds->GetCell(0);

      int intersection = cell->IntersectWithLine(
        this->PointA, this->PointB, IntersectionTolerance, t, this->Intersection, pcoord, subId);
      if (intersection == 0 && t == VTK_DOUBLE_MAX)
      {
        this->Intersection[0] = this->Intersection[1] = this->Intersection[2] =
          std::numeric_limits<double>::quiet_NaN();
        this->IntersectionNormal[0] = this->IntersectionNormal[1] = this->IntersectionNormal[2] =
          std::numeric_limits<double>::quiet_NaN();
        vtkErrorMacro("The intersection was not properly found");
        return;
      }

      std::vector<double> weights;
      weights.resize(cell->GetNumberOfPoints());
      cell->EvaluateLocation(subId, pcoord, x, weights.data());

      if (!vtkPVRayCastPickingHelper::ComputeSurfaceNormal(ds, cell, subId, weights.data()))
      {
        this->IntersectionNormal[0] = this->IntersectionNormal[1] = this->IntersectionNormal[2] =
          std::numeric_limits<double>::quiet_NaN();
      }
    }
  }
}
