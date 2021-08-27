#include "FEDataStructures.h"

#include <iostream>
#include <iterator>
#include <mpi.h>

Grid::Grid() = default;

void Grid::Initialize(const unsigned int numPoints[2], const double spacing[2])
{
  if (numPoints[0] == 0 || numPoints[1] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
  }

  this->Points.clear();

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
      // add the coordinate to the end of the vector
      std::copy(coord, coord + 3, std::back_inserter(this->Points));
    }
  }
  // create the quad cells
  std::vector<unsigned int> cellPoints(4);
  unsigned int numXPoints = endXPoint - startXPoint;
  for (unsigned int i = 0; i < numXPoints - 1; i++)
  {
    for (unsigned int j = 0; j < numPoints[1] - 1; j++)
    {
      cellPoints[0] = i * numPoints[1] + j;
      cellPoints[1] = (i + 1) * numPoints[1] + j;
      cellPoints[2] = (i + 1) * numPoints[1] + j + 1;
      cellPoints[3] = i * numPoints[1] + j + 1;

      this->PolygonalCells.AddElement(nullptr, cellPoints);
    }
  }
}

size_t Grid::GetNumberOfPoints() const
{
  return this->Points.size() / 3;
}
size_t Grid::GetNumberOfCells() const
{
  return this->PolygonalCells.GetNumberOfElements();
}

const double* Grid::GetPoint(size_t pointId) const
{
  if (pointId >= this->Points.size())
  {
    return nullptr;
  }
  return &(this->Points[pointId * 3]);
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
    const double* coord = this->GridPtr->GetPoint(pt);
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
