#include <iostream>
#include <iterator>
#include <mpi.h>
#include <stdlib.h>

#include "FEDataStructures.h"

Particles::Particles(Grid& grid, size_t numParticlesPerProcess)
{
  grid.GetGlobalBounds(this->GlobalBounds);
  this->Coordinates.resize(3 * numParticlesPerProcess);
  srand(0);
  this->Advect();
}

void Particles::Advect()
{
  int delta[3] = { static_cast<int>(this->GlobalBounds[1] - this->GlobalBounds[0]),
    static_cast<int>(this->GlobalBounds[3] - this->GlobalBounds[2]),
    static_cast<int>(this->GlobalBounds[5] - this->GlobalBounds[4]) };
  for (size_t i = 0; i < this->Coordinates.size() / 3; i++)
  {
    for (size_t j = 0; j < 3; j++)
    {
      this->Coordinates[3 * i + j] = static_cast<double>(rand() % delta[j]);
    }
  }
}

Grid::Grid(const unsigned int numPoints[3], const double spacing[3])
{
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }
  this->GlobalBounds[0] = this->GlobalBounds[2] = this->GlobalBounds[4] = 0;
  for (int i = 0; i < 3; i++)
  {
    this->GlobalBounds[1 + 2 * i] = (numPoints[i] - 1) * spacing[i];
  }
  // in parallel, we do a simple partitioning in the x-direction.
  int mpiSize = 1;
  int mpiRank = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);

  unsigned int startXPoint = mpiRank * numPoints[0] / mpiSize;
  unsigned int endXPoint = (mpiRank + 1) * numPoints[0] / mpiSize;
  if (mpiSize != mpiRank + 1)
  {
    endXPoint++;
  }

  // create the points -- slowest in the x and fastest in the z directions
  double coord[3] = { 0, 0, 0 };
  for (unsigned int i = startXPoint; i < endXPoint; i++)
  {
    coord[0] = i * spacing[0];
    for (unsigned int j = 0; j < numPoints[1]; j++)
    {
      coord[1] = j * spacing[1];
      for (unsigned int k = 0; k < numPoints[2]; k++)
      {
        coord[2] = k * spacing[2];
        // add the coordinate to the end of the vector
        std::copy(coord, coord + 3, std::back_inserter(this->Points));
      }
    }
  }
  // create the hex cells
  unsigned int cellPoints[8];
  unsigned int numXPoints = endXPoint - startXPoint;
  for (unsigned int i = 0; i < numXPoints - 1; i++)
  {
    for (unsigned int j = 0; j < numPoints[1] - 1; j++)
    {
      for (unsigned int k = 0; k < numPoints[2] - 1; k++)
      {
        cellPoints[0] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[1] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k;
        cellPoints[2] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[3] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k;
        cellPoints[4] = i * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[5] = (i + 1) * numPoints[1] * numPoints[2] + j * numPoints[2] + k + 1;
        cellPoints[6] = (i + 1) * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        cellPoints[7] = i * numPoints[1] * numPoints[2] + (j + 1) * numPoints[2] + k + 1;
        std::copy(cellPoints, cellPoints + 8, std::back_inserter(this->Cells));
      }
    }
  }
}

size_t Grid::GetNumberOfPoints()
{
  return this->Points.size() / 3;
}

size_t Grid::GetNumberOfCells()
{
  return this->Cells.size() / 8;
}

double* Grid::GetPointsArray()
{
  if (this->Points.empty())
  {
    return nullptr;
  }
  return &(this->Points[0]);
}

double* Grid::GetPoint(size_t pointId)
{
  if (pointId >= this->Points.size())
  {
    return nullptr;
  }
  return &(this->Points[pointId * 3]);
}

unsigned int* Grid::GetCellPoints(size_t cellId)
{
  if (cellId >= this->Cells.size())
  {
    return nullptr;
  }
  return &(this->Cells[cellId * 8]);
}

void Grid::GetGlobalBounds(double globalBounds[6])
{
  for (int i = 0; i < 6; i++)
  {
    globalBounds[i] = this->GlobalBounds[i];
  }
}

Attributes::Attributes()
{
  this->GridPtr = nullptr;
}

void Attributes::Initialize(Grid* grid)
{
  this->GridPtr = grid;
}

void Attributes::UpdateFields(double time)
{
  size_t numPoints = this->GridPtr->GetNumberOfPoints();
  this->Velocity.resize(numPoints * 3);
  for (size_t pt = 0; pt < numPoints; pt++)
  {
    double* coord = this->GridPtr->GetPoint(pt);
    this->Velocity[pt] = coord[1] * time;
  }
  std::fill(this->Velocity.begin() + numPoints, this->Velocity.end(), 0.);
  size_t numCells = this->GridPtr->GetNumberOfCells();
  this->Pressure.resize(numCells);
  std::fill(this->Pressure.begin(), this->Pressure.end(), 1.f);
}

double* Attributes::GetVelocityArray()
{
  if (this->Velocity.empty())
  {
    return nullptr;
  }
  return &this->Velocity[0];
}

float* Attributes::GetPressureArray()
{
  if (this->Pressure.empty())
  {
    return nullptr;
  }
  return &this->Pressure[0];
}
