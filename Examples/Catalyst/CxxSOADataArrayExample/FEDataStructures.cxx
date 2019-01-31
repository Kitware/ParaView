#include "FEDataStructures.h"

#include <iostream>
#include <iterator>
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
  // all of the x coordinates are stored first, then y coordinates and
  // finally z coordinates (e.g. x[0], x[1], ..., x[n-1], y[0], y[1], ...,
  // y[n-1], z[0], z[1], ..., z[n-1]) which is OPPOSITE of VTK's ordering.
  size_t numTotalPoints = (endXPoint - startXPoint) * numPoints[1] * numPoints[2];
  this->Points.resize(3 * numTotalPoints);
  size_t counter = 0;
  for (unsigned int i = startXPoint; i < endXPoint; i++)
  {
    for (unsigned int j = 0; j < numPoints[1]; j++)
    {
      for (unsigned int k = 0; k < numPoints[2]; k++)
      {
        this->Points[counter] = i * spacing[0];
        this->Points[numTotalPoints + counter] = j * spacing[1];
        this->Points[2 * numTotalPoints + counter] = k * spacing[2];
        counter++;
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
    return NULL;
  }
  return &(this->Points[0]);
}

bool Grid::GetPoint(size_t pointId, double coord[3])
{
  if (pointId >= this->Points.size() / 3)
  {
    return false;
  }
  coord[0] = this->Points[pointId];
  coord[1] = this->Points[pointId + this->GetNumberOfPoints()];
  coord[2] = this->Points[pointId + 2 * this->GetNumberOfPoints()];
  return true;
}

unsigned int* Grid::GetCellPoints(size_t cellId)
{
  if (cellId >= this->Cells.size())
  {
    return NULL;
  }
  return &(this->Cells[cellId * 8]);
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
  size_t numPoints = this->GridPtr->GetNumberOfPoints();
  this->Velocity.resize(numPoints * 3);
  double coord[3] = { 0, 0, 0 };
  for (size_t pt = 0; pt < numPoints; pt++)
  {
    this->GridPtr->GetPoint(pt, coord);
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
