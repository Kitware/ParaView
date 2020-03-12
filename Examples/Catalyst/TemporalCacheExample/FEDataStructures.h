/**
The code for the toy simulation itself is here. In this example we
make a configurable number of random sized spheres, and make them
bounce around in a cube. At each timestep we move the spheres, and then
sample them onto a configurable sized volume. The points and the volume
are available to Catalyst pipelines, but in this example only the
volume is temporally cached.

Note: Interaction between spheres is left as an exercise to the reader.
*/

#ifndef FEDATASTRUCTURES_HEADER
#define FEDATASTRUCTURES_HEADER

#include <cstddef>
#include <vector>

class particle;
class region;

class Grid
{
  // computational domain
public:
  ~Grid();
  Grid();
  void Initialize(const unsigned int numPoints[3], const double spacing[3]);
  unsigned int GetNumberOfLocalPoints();
  unsigned int GetNumberOfLocalCells();
  void GetLocalPoint(unsigned int pointId, double* point);
  unsigned int* GetNumPoints();
  unsigned int* GetExtent();
  double* GetSpacing();
  region* GetMyRegion() { return MyRegion; };

private:
  unsigned int NumPoints[3];
  unsigned int Extent[6];
  double Spacing[3];
  region* MyRegion;
};

class Attributes
{
  // array of results, updated every timestep
public:
  ~Attributes();
  Attributes(int numParticles);
  void Initialize(Grid* grid);
  void UpdateFields(double time);
  double* GetOccupancyArray();
  const std::vector<double>& GetParticles();

private:
  std::vector<double> Occupancy;
  std::vector<double> Particles;
  Grid* GridPtr;
  particle** MyParticles;
  int NumParticles;
};
#endif
