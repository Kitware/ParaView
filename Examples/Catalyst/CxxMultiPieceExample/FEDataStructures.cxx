#include "FEDataStructures.h"

#include <iostream>
#include <mpi.h>

Grid::Grid()
{
}

void Grid::Initialize(const unsigned int numPoints[3], const double spacing[3])
{
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }
  for (int i = 0; i < 3; i++)
  {
    this->Extents[i * 2] = 0;
    this->Extents[i * 2 + 1] = numPoints[i];
    this->Spacing[i] = spacing[i];
  }

  // in parallel, we do a simple partitioning in the x-direction.
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  this->Extents[0] = mpiRank * numPoints[0] / mpiSize;
  this->Extents[1] = (mpiRank + 1) * numPoints[0] / mpiSize;
}

unsigned int Grid::GetNumberOfLocalPoints()
{
  return (this->Extents[1] - this->Extents[0] + 1) * (this->Extents[3] - this->Extents[2] + 1) *
    (this->Extents[5] - this->Extents[4] + 1);
}

unsigned int Grid::GetNumberOfLocalCells()
{
  return (this->Extents[1] - this->Extents[0]) * (this->Extents[3] - this->Extents[2]) *
    (this->Extents[5] - this->Extents[4]);
}

void Grid::GetPoint(unsigned int logicalLocalCoords[3], double coord[3])
{
  coord[0] = (this->Extents[0] + logicalLocalCoords[0]) * this->Spacing[0];
  coord[1] = logicalLocalCoords[1] * this->Spacing[1];
  coord[2] = logicalLocalCoords[2] * this->Spacing[2];
}

double* Grid::GetSpacing()
{
  return this->Spacing;
}

unsigned int* Grid::GetExtents()
{
  return this->Extents;
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
  size_t numPoints = this->GridPtr->GetNumberOfLocalPoints();
  this->Velocity.resize(numPoints * 3);
  unsigned int* extents = this->GridPtr->GetExtents();
  unsigned int logicalLocalCoords[3];
  size_t pt = 0;
  for (unsigned int k = 0; k < extents[5] - extents[4] + 1; k++)
  {
    logicalLocalCoords[2] = k;
    for (unsigned int j = 0; j < extents[3] - extents[2] + 1; j++)
    {
      logicalLocalCoords[1] = j;
      for (unsigned int i = 0; i < extents[1] - extents[0] + 1; i++)
      {
        logicalLocalCoords[0] = i;
        double coord[3];
        this->GridPtr->GetPoint(logicalLocalCoords, coord);
        this->Velocity[pt] = coord[1] * time;
        pt++;
      }
    }
  }
  std::fill(this->Velocity.begin() + numPoints, this->Velocity.end(), 0.);
  size_t numCells = this->GridPtr->GetNumberOfLocalCells();
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
