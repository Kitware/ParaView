#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <cstddef>
#include <vector>

class Grid
{
public:
  Grid(const unsigned int numPoints[3], const double spacing[3]);
  size_t GetNumberOfPoints();
  size_t GetNumberOfCells();
  double* GetPointsArray();
  double* GetPoint(size_t pointId);
  unsigned int* GetCellPoints(size_t cellId);
  void GetGlobalBounds(double globalBounds[6]);

private:
  std::vector<double> Points;
  std::vector<unsigned int> Cells;
  double GlobalBounds[6];
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

class Particles
{
public:
  Particles(Grid& grid, size_t numParticlesPerProcess);
  void Advect();
  double* GetPointsArray() { return &this->Coordinates[0]; }
  size_t GetNumberOfPoints() { return this->Coordinates.size() / 3; }

private:
  std::vector<double> Coordinates;
  double GlobalBounds[6];
};
#endif
