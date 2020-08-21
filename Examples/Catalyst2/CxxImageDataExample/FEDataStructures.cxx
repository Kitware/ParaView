#include "FEDataStructures.h"

#include <assert.h>
#include <iostream>
#include <mpi.h>

Grid::Grid()
{
  this->NumPoints[0] = this->NumPoints[1] = this->NumPoints[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
}

void Grid::Initialize(const unsigned int numPoints[3], const double spacing[3])
{
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }
  for (int i = 0; i < 3; i++)
  {
    this->NumPoints[i] = numPoints[i];
    this->Spacing[i] = spacing[i];
  }
  int mpiRank = 0, mpiSize = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  this->Extent[0] = mpiRank * numPoints[0] / mpiSize;
  this->Extent[1] = (mpiRank + 1) * numPoints[0] / mpiSize;
  if (mpiSize != mpiRank + 1)
  {
    this->Extent[1]++;
  }
  this->Extent[2] = this->Extent[4] = 0;
  this->Extent[3] = numPoints[1];
  this->Extent[5] = numPoints[2];
}

unsigned int Grid::GetNumberOfLocalPoints()
{
  return (this->Extent[1] - this->Extent[0] + 1) * (this->Extent[3] - this->Extent[2] + 1) *
    (this->Extent[5] - this->Extent[4] + 1);
}

unsigned int Grid::GetNumberOfLocalCells()
{
  return (this->Extent[1] - this->Extent[0]) * (this->Extent[3] - this->Extent[2]) *
    (this->Extent[5] - this->Extent[4]);
}

void Grid::GetLocalPoint(unsigned int pointId, double* point)
{
  unsigned int logicalX = pointId % (this->Extent[1] - this->Extent[0] + 1);
  assert(logicalX <= this->Extent[1]);
  point[0] = this->Spacing[0] * logicalX;
  unsigned int logicalY =
    pointId % ((this->Extent[1] - this->Extent[0] + 1) * (this->Extent[3] - this->Extent[2] + 1));
  logicalY /= this->Extent[1] - this->Extent[0] + 1;
  assert(logicalY <= this->Extent[3]);
  point[1] = this->Spacing[1] * logicalY;
  unsigned int logicalZ =
    pointId / ((this->Extent[1] - this->Extent[0] + 1) * (this->Extent[3] - this->Extent[2] + 1));
  assert(logicalZ <= this->Extent[5]);
  point[2] = this->Spacing[2] * logicalZ;
}

unsigned int* Grid::GetNumPoints()
{
  return this->NumPoints;
}

unsigned int* Grid::GetExtent()
{
  return this->Extent;
}

double* Grid::GetSpacing()
{
  return this->Spacing;
}

Attributes::Attributes()
{
  this->GridPtr = NULL;
}

void Attributes::Initialize(Grid* grid)
{
  this->GridPtr = grid;
}

void Attributes::UpdateFields(double time)
{
  unsigned int numPoints = this->GridPtr->GetNumberOfLocalPoints();
  this->Velocity.resize(numPoints * 3);
  for (unsigned int pt = 0; pt < numPoints; pt++)
  {
    double coord[3];
    this->GridPtr->GetLocalPoint(pt, coord);
    this->Velocity[pt] = coord[1] * time;
  }
  std::fill(this->Velocity.begin() + numPoints, this->Velocity.end(), 0.);
  unsigned int numCells = this->GridPtr->GetNumberOfLocalCells();
  this->Pressure.resize(numCells);
  std::fill(this->Pressure.begin(), this->Pressure.end(), 1.f);
}

double* Attributes::GetVelocityArray()
{
  if (this->Velocity.empty())
  {
    return NULL;
  }
  return &this->Velocity[0];
}

float* Attributes::GetPressureArray()
{
  if (this->Pressure.empty())
  {
    return NULL;
  }
  return &this->Pressure[0];
}
