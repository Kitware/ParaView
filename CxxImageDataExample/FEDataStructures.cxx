#include "FEDataStructures.h"

#include <iostream>

Grid::Grid()
{
  this->NumPoints[0] = this->NumPoints[1] = this->NumPoints[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
}

void Grid::Initialize(const unsigned int numPoints[3], const double spacing[3] )
{
  if(numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
    {
    std::cerr << "Must have a non-zero amount of points in each direction.\n";
    }
  for(int i=0;i<3;i++)
    {
    this->NumPoints[i] = numPoints[i];
    this->Spacing[i] = spacing[i];
    }
}

unsigned int Grid::GetNumberOfPoints()
{
  return this->NumPoints[0]*this->NumPoints[1]*this->NumPoints[2];
}

size_t Grid::GetNumberOfCells()
{
  return (this->NumPoints[0]-1)*(this->NumPoints[1]-1)*(this->NumPoints[2]-1);
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
  this->Velocity.resize(numPoints*3);
  for(size_t pt=0;pt<numPoints;pt++)
    {
    double* coord = this->GridPtr->GetPoint(pt);
    this->Velocity[pt] = coord[1]*time;
    }
  std::fill(this->Velocity.begin()+numPoints, this->Velocity.end(), 0.);
  size_t numCells = this->GridPtr->GetNumberOfCells();
  this->Pressure.resize(numCells);
  std::fill(this->Pressure.begin(), this->Pressure.end(), 1.);
}

double* Attributes::GetVelocityArray()
{
  if(this->Velocity.empty())
    {
    return NULL;
    }
  return &this->Velocity[0];
}

float* Attributes::GetPressureArray()
{
  if(this->Pressure.empty())
    {
    return NULL;
    }
  return &this->Pressure[0];
}
