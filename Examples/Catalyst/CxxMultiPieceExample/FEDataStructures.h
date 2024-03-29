// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <cstddef>
#include <vector>

class Grid
{
public:
  Grid();
  void Initialize(const unsigned int numGlobalPoints[3], const double spacing[3]);
  unsigned int GetNumberOfLocalPoints();
  unsigned int GetNumberOfLocalCells();
  void GetPoint(unsigned int logicalLocalCoords[3], double coord[3]);
  double* GetSpacing();
  unsigned int* GetExtents();

private:
  unsigned int Extents[6];
  double Spacing[3];
};

class Attributes
{
  // A class for generating and storing point and cell fields.
  // Velocity is stored at the points and pressure is stored
  // for the cells. The current velocity profile is for a
  // shearing flow with U(y,t) = y*t, V = 0 and W = 0.
  // Pressure is constant through the domain.
public:
  Attributes();
  void Initialize(Grid* grid);
  void UpdateFields(double time);
  double* GetVelocityArray();
  float* GetPressureArray();

private:
  std::vector<double> Velocity;
  std::vector<float> Pressure;
  Grid* GridPtr;
};
#endif
