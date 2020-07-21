#include "FEDataStructures.h"

#include <assert.h>
#include <iostream>
#include <math.h>
#include <mpi.h>
#include <string.h>

#include <vtkMath.h>

class particle
{
  // A sphere, moving in some direction
public:
  double pos[3];
  double vel[3];
  double radius;
  int id;
  static double world[];

public:
  particle() = delete;
  particle(int i, double r, double x, double y, double z, double vx, double vy, double vz)
    : id(i)
    , radius(r)
    , pos{ x, y, z }
    , vel{ vx, vy, vz }
  {
  }
  static void setworld(double x0, double x1, double y0, double y1, double z0, double z1)
  {
    world[0] = x0;
    world[1] = x1;
    world[2] = y0;
    world[3] = y1;
    world[4] = z0;
    world[5] = z1;
  }
  void print() { std::cout << id << ": " << pos[0] << "," << pos[1] << "," << pos[2] << std::endl; }
  void update()
  {
    double n;
    for (int c = 0; c < 3; c++)
    {
      n = pos[c] + vel[c];
      // spheres will bound off the walls
      if (n < world[c * 2 + 0] || n > world[c * 2 + 1])
      {
        vel[c] *= -1;
      }
      pos[c] += vel[c];
    }
  }
  double distance(double x, double y, double z)
  {
    double r = sqrt((x - pos[0]) * (x - pos[0]) + (y - pos[1]) * (y - pos[1]) +
                 (z - pos[2]) * (z - pos[2])) -
      radius;
    return r;
  }
  bool bbox_test(double x0, double x1, double y0, double y1, double z0, double z1)
  {
    if ((pos[0] + radius) < x0)
      return false;
    if ((pos[0] - radius) > x1)
      return false;
    if ((pos[1] + radius) < y0)
      return false;
    if ((pos[1] - radius) > y1)
      return false;
    if ((pos[2] + radius) < z0)
      return false;
    if ((pos[2] - radius) > z1)
      return false;
    return true;
  }
  void getCharacteristics(double* ret)
  {
    ret[0] = pos[0];
    ret[1] = pos[1];
    ret[2] = pos[2];
    ret[3] = radius;
    ret[4] = id;
  }
};

// geometric extent of the box that the particles move in
double particle::world[6] = { 0.0, 1.0, 0.0, 1.0, 0.0, 1.0 };

class region
{
  // a region of space that an MPI rank is responsible for
private:
  double origin[3];
  double spacing[3];
  int extent[6];
  int memsize;
  double* values;
  double x0, x1, y0, y1, z0, z1;
  std::vector<double> myparticles;

public:
  region() = delete;
  region(double x, double y, double z, double sx, double sy, double sz, int i0, int i1, int j0,
    int j1, int k0, int k1)
    : origin{ x, y, x }
    , spacing{ sx, sy, sz }
    , extent{ i0, i1, j0, j1, k0, k1 }
  {
    memsize = (extent[1] - extent[0]) * (extent[3] - extent[2]) * (extent[5] - extent[4]);
    values = new double[memsize];
    reset();
    x0 = origin[0] + extent[0] * spacing[0];
    x1 = origin[0] + extent[1] * spacing[0];
    y0 = origin[1] + extent[2] * spacing[1];
    y1 = origin[1] + extent[3] * spacing[1];
    z0 = origin[2] + extent[4] * spacing[2];
    z1 = origin[2] + extent[5] * spacing[2];
  }
  void reset()
  {
    memset((void*)values, 0, memsize * sizeof(double));
    myparticles.clear();
  }
  void accumulate(particle p)
  {
    // sample this particle onto the volume
    if (p.bbox_test(x0, x1, y0, y1, z0, z1))
    {
      // std::cout << getpid() << " hit" << std::endl;
      double chars[5];
      p.getCharacteristics(chars);
      myparticles.push_back(chars[0]); // x
      myparticles.push_back(chars[1]); // y
      myparticles.push_back(chars[2]); // z
      myparticles.push_back(chars[3]); // r
      myparticles.push_back(chars[4]); // id
      double* value = values;
      int i0 = static_cast<int>(((p.pos[0] - p.radius) + origin[0]) / spacing[0]);
      int i1 = static_cast<int>(((p.pos[0] + p.radius) + origin[0]) / spacing[0]);
      int j0 = static_cast<int>(((p.pos[1] - p.radius) + origin[1]) / spacing[1]);
      int j1 = static_cast<int>(((p.pos[1] + p.radius) + origin[1]) / spacing[1]);
      int k0 = static_cast<int>(((p.pos[2] - p.radius) + origin[2]) / spacing[2]);
      int k1 = static_cast<int>(((p.pos[2] + p.radius) + origin[2]) / spacing[2]);
      int di = extent[1] - extent[0];
      int dj = extent[3] - extent[2];
      int dk = extent[5] - extent[4];
      for (int i = i0; i < i1; i++)
      {
        if (i < 0 || i < extent[0] || i >= extent[1])
          continue;
        for (int j = j0; j < j1; j++)
        {
          if (j < 0 || j < extent[2] || j >= extent[3])
            continue;
          for (int k = k0; k < k1; k++)
          {
            if (k < 0 || k < extent[4] || k >= extent[5])
              continue;
            double x = origin[0] + i * spacing[0];
            double y = origin[1] + j * spacing[1];
            double z = origin[2] + k * spacing[2];
            if (p.distance(x, y, z) <= 0.0)
            {
              value = values + i * dj * dk + j * dk + k;
              *value = *value + 1.0;
            }
          }
        }
      }
    }
  }
  double* getValues() { return this->values; };
  const std::vector<double>& getParticles() { return myparticles; };
};

Grid::Grid()
{
  this->NumPoints[0] = this->NumPoints[1] = this->NumPoints[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->MyRegion = nullptr;
}

Grid::~Grid()
{
  delete this->MyRegion;
}

void Grid::Initialize(const unsigned int numPoints[3], const double spacing[3])
{
  if (numPoints[0] == 0 || numPoints[1] == 0 || numPoints[2] == 0)
  {
    std::cerr << "Must have a non-zero amount of points in each direction." << std::endl;
    return;
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

  // every rank knows of the entire space
  particle::setworld(0, numPoints[2], 0, numPoints[1], 0, numPoints[0]);
  // every rank has only its own region of space
  this->MyRegion = new region(0, 0, 0, spacing[2], spacing[1], spacing[0], this->Extent[4],
    this->Extent[5], this->Extent[2], this->Extent[3], this->Extent[0], this->Extent[1]);
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

Attributes::Attributes(int numparticles)
{
  this->GridPtr = nullptr;
  this->NumParticles = numparticles;
  this->MyParticles = new particle*[this->NumParticles];
  for (int i = 0; i < this->NumParticles; i++)
  {
    this->MyParticles[i] = nullptr;
  }
}

Attributes::~Attributes()
{
  for (int i = 0; i < this->NumParticles; i++)
  {
    delete this->MyParticles[i];
  }
  delete[] this->MyParticles;
}

void Attributes::Initialize(Grid* grid)
{
  this->GridPtr = grid;
  double* cellspacing = grid->GetSpacing();
  double cellsize = sqrt(cellspacing[0] * cellspacing[0] + cellspacing[1] * cellspacing[1] +
    cellspacing[2] * cellspacing[2]);
  unsigned int* npts = grid->GetNumPoints();
  // world extent to place particles within
  double x0 = 0;
  double x1 = npts[2] * cellspacing[2];
  double y0 = 0;
  double y1 = npts[1] * cellspacing[1];
  double z0 = 0;
  double z1 = npts[0] * cellspacing[0];

// a tuning parameter which keeps sizes relatively good in cases I've tried
#define ADJF 1.5
  vtkMath::RandomSeed(42);
  // every rank has every particle just to keep the simulation simple
  // in real life you would want the particles to live with the processes
  for (int i = 0; i < this->NumParticles; i++)
  {
    switch (i)
    {
      case 0:
        this->MyParticles[0] = new particle(0, 2 * cellsize * ADJF, (x0 + x1) / 2, (y0 + y1) / 2,
          z0, 0.0, 0.0, cellsize * 0.5 * ADJF);
        break;
      case 1:
        this->MyParticles[1] = new particle(1, 3 * cellsize * ADJF, (x0 + x1) / 2, (y0 + y1) / 2,
          z1, 0.0, 0.0, -cellsize * 0.5 * ADJF);
        break;
      default:
        double r = vtkMath::Random() * 1.5 * cellsize * ADJF;
        double x = vtkMath::Random() * (x1 - x0) + x0;
        double y = vtkMath::Random() * (y1 - y0) + y0;
        double z = vtkMath::Random() * (z1 - z0) + z0;
        double vx = (2.0 * vtkMath::Random() - 1.0) * cellsize * 2.0 * ADJF;
        double vy = (2.0 * vtkMath::Random() - 1.0) * cellsize * 2.0 * ADJF;
        double vz = (2.0 * vtkMath::Random() - 1.0) * cellsize * 2.0 * ADJF;
        this->MyParticles[i] = new particle(i, r, x, y, z, vx, vy, vz);
    }
  }
}

void Attributes::UpdateFields(double time)
{
  // the is the main entry point for each timestep
  unsigned int numPoints = this->GridPtr->GetNumberOfLocalPoints();
  unsigned int numCells = this->GridPtr->GetNumberOfLocalCells();
  this->Occupancy.resize(numCells);

  // start off clear for this timestep
  std::fill(this->Occupancy.begin(), this->Occupancy.end(), 0.0);
  region* r = this->GridPtr->GetMyRegion();
  r->reset();

  // move all of the particles
  for (int i = 0; i < this->NumParticles; i++)
  {
    this->MyParticles[i]->update();
  }
  // discretize all of the particles onto the cells of the volume
  for (int i = 0; i < this->NumParticles; i++)
  {
    r->accumulate(*this->MyParticles[i]);
  }
  // transfer them to the array I am going to output
  double* value = r->getValues();
  for (int c = 0; c < this->Occupancy.size(); c++)
  {
    this->Occupancy[c] = *value;
    value++;
  }
  this->Particles = r->getParticles();
}

double* Attributes::GetOccupancyArray()
{
  if (this->Occupancy.empty())
  {
    return nullptr;
  }
  return &this->Occupancy[0];
}

const std::vector<double>& Attributes::GetParticles()
{
  return this->Particles;
}
