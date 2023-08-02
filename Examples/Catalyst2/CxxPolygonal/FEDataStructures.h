// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <vector>

// one 2 many relationship storage.
struct O2MRelation
{
  std::vector<unsigned int> Connectivity;
  std::vector<unsigned int> Sizes;
  std::vector<unsigned int> Offsets;

  size_t GetNumberOfElements() const { return this->Sizes.size(); }

  unsigned int AddElement(const unsigned int* ptIds, const std::vector<unsigned int>& lids)
  {
    auto index = this->Sizes.size();
    assert(this->Sizes.size() == this->Offsets.size());

    this->Sizes.push_back(static_cast<unsigned int>(lids.size()));
    this->Offsets.push_back(static_cast<unsigned int>(this->Connectivity.size()));

    this->Connectivity.resize(this->Connectivity.size() + lids.size());
    std::transform(lids.begin(), lids.end(),
      std::next(this->Connectivity.begin(), this->Offsets.back()),
      [&ptIds](unsigned int idx) { return ptIds ? ptIds[idx] : idx; });

    return index;
  }
};

class Grid
{
public:
  Grid();
  void Initialize(const unsigned int numPoints[2], const double spacing[2]);

  size_t GetNumberOfPoints() const;
  size_t GetNumberOfCells() const;

  const double* GetPoint(size_t id) const;
  const std::vector<double>& GetPoints() const { return this->Points; }
  const O2MRelation& GetPolygonalCells() const { return this->PolygonalCells; }

private:
  std::vector<double> Points;
  O2MRelation PolygonalCells;
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
