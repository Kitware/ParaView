/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellIntegrator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellIntegrator.h"

#include "vtkCell.h"
#include "vtkDataSet.h"
#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkPoints.h"

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegratePolyLine(
  vtkDataSet* input, vtkIdType vtkNotUsed(cellId), vtkIdList* ptIds)
{
  double length;
  double pt1[3], pt2[3];
  vtkIdType numLines, lineIdx;
  vtkIdType pt1Id, pt2Id;

  double totalLength = 0;
  numLines = ptIds->GetNumberOfIds() - 1;
  for (lineIdx = 0; lineIdx < numLines; ++lineIdx)
  {
    pt1Id = ptIds->GetId(lineIdx);
    pt2Id = ptIds->GetId(lineIdx + 1);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    totalLength += length;
  }
  return totalLength;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateGeneral1DCell(
  vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds)
{
  // Determine the number of lines
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be an even number of points from the triangulation
  if (nPnts % 2)
  {
    vtkGenericWarningMacro("Odd number of points(" << nPnts << ")  encountered - skipping "
                                                   << " 1D Cell: " << cellId);
    return 0;
  }

  double length;
  double pt1[3], pt2[3];
  vtkIdType pid = 0;
  vtkIdType pt1Id, pt2Id;

  double totalLength = 0;
  while (pid < nPnts)
  {
    pt1Id = ptIds->GetId(pid++);
    pt2Id = ptIds->GetId(pid++);
    input->GetPoint(pt1Id, pt1);
    input->GetPoint(pt2Id, pt2);

    // Compute the length of the line.
    length = sqrt(vtkMath::Distance2BetweenPoints(pt1, pt2));
    totalLength += length;
  }
  return totalLength;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateTriangleStrip(
  vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  double totalArea = 0;
  numTris = ptIds->GetNumberOfIds() - 2;
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt1Id = ptIds->GetId(triIdx);
    pt2Id = ptIds->GetId(triIdx + 1);
    pt3Id = ptIds->GetId(triIdx + 2);
    totalArea += vtkCellIntegrator::IntegrateTriangle(input, cellId, pt1Id, pt2Id, pt3Id);
  }
  return totalArea;
}

//-----------------------------------------------------------------------------
// Works for convex polygons, and interpoaltion is not correct.
double vtkCellIntegrator::IntegratePolygon(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds)
{
  vtkIdType numTris, triIdx;
  vtkIdType pt1Id, pt2Id, pt3Id;

  double totalArea = 0;
  numTris = ptIds->GetNumberOfIds() - 2;
  pt1Id = ptIds->GetId(0);
  for (triIdx = 0; triIdx < numTris; ++triIdx)
  {
    pt2Id = ptIds->GetId(triIdx + 1);
    pt3Id = ptIds->GetId(triIdx + 2);
    totalArea += vtkCellIntegrator::IntegrateTriangle(input, cellId, pt1Id, pt2Id, pt3Id);
  }
  return totalArea;
}

//-----------------------------------------------------------------------------
// For axis aligned rectangular cells
double vtkCellIntegrator::IntegratePixel(
  vtkDataSet* input, vtkIdType /*cellId*/, vtkIdList* cellPtIds)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;
  double pts[4][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double l, w, area;

  // get the lengths of its 2 orthogonal sides.  Since only 1 coordinate
  // can be different we can add the differences in all 3 directions
  l = (pts[0][0] - pts[1][0]) + (pts[0][1] - pts[1][1]) + (pts[0][2] - pts[1][2]);

  w = (pts[0][0] - pts[2][0]) + (pts[0][1] - pts[2][1]) + (pts[0][2] - pts[2][2]);

  area = fabs(l * w);
  return area;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateTriangle(vtkDataSet* input, vtkIdType vtkNotUsed(cellId),
  vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id)
{
  double pt1[3], pt2[3], pt3[3];
  double v1[3], v2[3];
  double cross[3];
  double area;

  input->GetPoint(pt1Id, pt1);
  input->GetPoint(pt2Id, pt2);
  input->GetPoint(pt3Id, pt3);

  // Compute two legs.
  v1[0] = pt2[0] - pt1[0];
  v1[1] = pt2[1] - pt1[1];
  v1[2] = pt2[2] - pt1[2];
  v2[0] = pt3[0] - pt1[0];
  v2[1] = pt3[1] - pt1[1];
  v2[2] = pt3[2] - pt1[2];

  // Use the cross product to compute the area of the parallelogram.
  vtkMath::Cross(v1, v2, cross);
  area = sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]) * 0.5;
  return area;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateGeneral2DCell(
  vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds)
{
  double totalArea = 0;
  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 3
  // from the triangulation
  if (nPnts % 3)
  {
    vtkGenericWarningMacro("Number of points (" << nPnts << ") is not divisiable by 3 - skipping "
                                                << " 2D Cell: " << cellId);
    return 0;
  }

  vtkIdType triIdx = 0;
  vtkIdType pt1Id, pt2Id, pt3Id;

  while (triIdx < nPnts)
  {
    pt1Id = ptIds->GetId(triIdx++);
    pt2Id = ptIds->GetId(triIdx++);
    pt3Id = ptIds->GetId(triIdx++);
    totalArea += vtkCellIntegrator::IntegrateTriangle(input, cellId, pt1Id, pt2Id, pt3Id);
  }
  return totalArea;
}

//-----------------------------------------------------------------------------
// For axis aligned hexahedral cells
double vtkCellIntegrator::IntegrateVoxel(
  vtkDataSet* input, vtkIdType vtkNotUsed(cellId), vtkIdList* cellPtIds)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id, pt5Id;
  double pts[5][3];
  pt1Id = cellPtIds->GetId(0);
  pt2Id = cellPtIds->GetId(1);
  pt3Id = cellPtIds->GetId(2);
  pt4Id = cellPtIds->GetId(3);
  pt5Id = cellPtIds->GetId(4);
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);
  input->GetPoint(pt5Id, pts[4]);

  double l, w, h;

  // Calculate the volume of the voxel
  l = pts[1][0] - pts[0][0];
  w = pts[2][1] - pts[0][1];
  h = pts[4][2] - pts[0][2];
  double volume = fabs(l * w * h);

  return volume;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateTetrahedron(vtkDataSet* input, vtkIdType vtkNotUsed(cellId),
  vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id)
{
  double pts[4][3];
  input->GetPoint(pt1Id, pts[0]);
  input->GetPoint(pt2Id, pts[1]);
  input->GetPoint(pt3Id, pts[2]);
  input->GetPoint(pt4Id, pts[3]);

  double a[3], b[3], c[3], n[3], volume;
  int i;
  // Compute the principle vectors around pt0 and the
  // centroid
  for (i = 0; i < 3; i++)
  {
    a[i] = pts[1][i] - pts[0][i];
    b[i] = pts[2][i] - pts[0][i];
    c[i] = pts[3][i] - pts[0][i];
  }

  // Calculate the volume of the tet which is 1/6 * the box product
  vtkMath::Cross(a, b, n);
  volume = vtkMath::Dot(c, n) / 6.0;
  return volume;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::IntegrateGeneral3DCell(
  vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds)
{
  double volume = 0;

  vtkIdType nPnts = ptIds->GetNumberOfIds();
  // There should be a number of points that is a multiple of 4
  // from the triangulation
  if (nPnts % 4)
  {
    vtkGenericWarningMacro("Number of points (" << nPnts << ") is not divisiable by 4 - skipping "
                                                << " 3D Cell: " << cellId);
    return 0;
  }

  vtkIdType tetIdx = 0;
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;

  while (tetIdx < nPnts)
  {
    pt1Id = ptIds->GetId(tetIdx++);
    pt2Id = ptIds->GetId(tetIdx++);
    pt3Id = ptIds->GetId(tetIdx++);
    pt4Id = ptIds->GetId(tetIdx++);
    volume += vtkCellIntegrator::IntegrateTetrahedron(input, cellId, pt1Id, pt2Id, pt3Id, pt4Id);
  }

  return volume;
}

//-----------------------------------------------------------------------------
double vtkCellIntegrator::Integrate(vtkDataSet* input, vtkIdType cellId)
{
  vtkIdType pt1Id, pt2Id, pt3Id, pt4Id;

  double sum = 0;

  int cellType = input->GetCellType(cellId);

  vtkPoints* cellPoints = 0;
  vtkIdList* cellPtIds = vtkIdList::New();

  switch (cellType)
  {
    // skip empty or 0D Cells
    case VTK_EMPTY_CELL:
    case VTK_VERTEX:
    case VTK_POLY_VERTEX:
      break;

    case VTK_POLY_LINE:
    case VTK_LINE:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegratePolyLine(input, cellId, cellPtIds);
      break;

    case VTK_TRIANGLE:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegrateTriangle(
        input, cellId, cellPtIds->GetId(0), cellPtIds->GetId(1), cellPtIds->GetId(2));
      break;

    case VTK_TRIANGLE_STRIP:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegrateTriangleStrip(input, cellId, cellPtIds);
      break;

    case VTK_POLYGON:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegratePolygon(input, cellId, cellPtIds);
      break;

    case VTK_PIXEL:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegratePixel(input, cellId, cellPtIds);
      break;

    case VTK_QUAD:
      input->GetCellPoints(cellId, cellPtIds);
      pt1Id = cellPtIds->GetId(0);
      pt2Id = cellPtIds->GetId(1);
      pt3Id = cellPtIds->GetId(2);
      sum += vtkCellIntegrator::IntegrateTriangle(input, cellId, pt1Id, pt2Id, pt3Id);
      pt2Id = cellPtIds->GetId(3);
      sum += vtkCellIntegrator::IntegrateTriangle(input, cellId, pt1Id, pt2Id, pt3Id);
      break;

    case VTK_VOXEL:
      input->GetCellPoints(cellId, cellPtIds);
      sum = vtkCellIntegrator::IntegrateVoxel(input, cellId, cellPtIds);
      break;

    case VTK_TETRA:
      input->GetCellPoints(cellId, cellPtIds);
      pt1Id = cellPtIds->GetId(0);
      pt2Id = cellPtIds->GetId(1);
      pt3Id = cellPtIds->GetId(2);
      pt4Id = cellPtIds->GetId(3);
      sum = vtkCellIntegrator::IntegrateTetrahedron(input, cellId, pt1Id, pt2Id, pt3Id, pt4Id);
      break;

    default:
      // We need to explicitly get the cell
      vtkCell* cell = input->GetCell(cellId);
      int cellDim = cell->GetCellDimension();
      if (cellDim == 0)
      {
        break;
      }

      // We will need a place to store points from the cell's
      // triangulate function
      if (!cellPoints)
      {
        cellPoints = vtkPoints::New();
      }

      cell->Triangulate(1, cellPtIds, cellPoints);
      switch (cellDim)
      {
        case 1:
          sum = vtkCellIntegrator::IntegrateGeneral1DCell(input, cellId, cellPtIds);
          break;
        case 2:
          sum = vtkCellIntegrator::IntegrateGeneral2DCell(input, cellId, cellPtIds);
          break;
        case 3:
          sum = vtkCellIntegrator::IntegrateGeneral3DCell(input, cellId, cellPtIds);
          break;
        default:
          vtkGenericWarningMacro("Unsupported Cell Dimension = " << cellDim);
      }
  }

  cellPtIds->Delete();
  if (cellPoints)
  {
    cellPoints->Delete();
  }

  return sum;
}

//----------------------------------------------------------------------------
void vtkCellIntegrator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
