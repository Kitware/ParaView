/*=========================================================================

  Program:   ParaView
  Module:    Grid.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "Grid.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

namespace
{
const double sqrt3 = sqrt(3);
const int NFACES = 6;
const double MAX_DIFF = 1e-10;
const double PI = atan(1) * 4;

//==============================================================================
// Returns true if a < b. First it does a relative comparison between a and b
// and returns false if they are almost equal. This does not work around 0.
// maxDiff is DBL_EPSILON or small multiples of it.
// For more information about floating point comparison see
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
class FuzzyEQ
{
public:
  FuzzyEQ(double maxDiff)
    : MaxDiff(maxDiff)
  {
  }

  bool operator()(double a, double b) const
  {
    double diff = fabs(a - b);
    return diff <= this->MaxDiff;
  }

private:
  const double MaxDiff;
};

class FuzzyLT
{
public:
  FuzzyLT(double maxDiff)
    : MaxDiff(maxDiff)
  {
  }

  bool operator()(double a, double b) const
  {
    FuzzyEQ eq(this->MaxDiff);
    if (eq(a, b))
    {
      return false;
    }
    return a < b;
  }

private:
  const double MaxDiff;
};

//==============================================================================
class FuzzyLT1E_10 : public FuzzyLT
{
public:
  FuzzyLT1E_10()
    : FuzzyLT(MAX_DIFF)
  {
  }
};

//------------------------------------------------------------------------------
// Creates an array for attribute 'name' and adds it to 'grid'.
template <typename T>
static int addAttribute(
  vtkSmartPointer<vtkUnstructuredGrid> grid, const char* name, vtkIdType size, int nComponents)
{
  vtkSmartPointer<T> a = vtkSmartPointer<T>::New();
  a->SetNumberOfComponents(nComponents);
  a->SetNumberOfTuples(size);
  a->SetName(name);
  return grid->GetCellData()->AddArray(a);
}

//------------------------------------------------------------------------------
void rotateAroundY(double q, const double value[3], double r[3])
{
  std::vector<double> v(value, value + 3);
  double rotY[3][3] = { { cos(q), 0, sin(q) }, { 0, 1, 0 }, { -sin(q), 0, cos(q) } };
  vtkMath::Multiply3x3(rotY, &v[0], r);
}

//------------------------------------------------------------------------------
void rotateAroundYDeg(double degQ, const double value[3], double r[3])
{
  double q = vtkMath::RadiansFromDegrees(degQ);
  rotateAroundY(q, value, r);
}

//------------------------------------------------------------------------------
void rotateAroundZ(double q, const double value[3], double r[3])
{
  std::vector<double> v(value, value + 3);
  double rotZ[3][3] = { { cos(q), -sin(q), 0 }, { sin(q), cos(q), 0 }, { 0, 0, 1 } };
  vtkMath::Multiply3x3(rotZ, &v[0], r);
}

//------------------------------------------------------------------------------
void rotateAroundZDeg(double degQ, const double value[3], double r[3])
{
  double q = vtkMath::RadiansFromDegrees(degQ);
  rotateAroundZ(q, value, r);
}

//------------------------------------------------------------------------------
// This function maps (lonDeg, latDeg, r) to (x,y,z) on
// the sphere of radius r.
// The sequence or rotations and sign were done to match the history file.
// They use right coordinate system, with Z up, Y toward the screen, X to the right
void sphericalDegToCartesian(double v[3])
{
  double lonDeg = v[0];
  double latDeg = v[1];
  double src[3] = { v[2], 0, 0 };
  double r[3];
  rotateAroundYDeg(-latDeg, src, r);
  rotateAroundZDeg(lonDeg, r, v);
  // std::cerr << "lonDeg=" << lonDeg
  //           << " latDeg=" << latDeg
  //           << " x=" << src[0]
  //           << " xp=" << v[0]
  //           << " yp=" << v[1]
  //           << " zp=" << v[2] << endl;
}

//------------------------------------------------------------------------------
void sphericalToCartesian(double v[3])
{
  double lonRad = v[0];
  double latRad = v[1];
  double src[3] = { v[2], 0, 0 };
  double r[3];
  rotateAroundY(-latRad, src, r);
  rotateAroundZ(lonRad, r, v);
  // std::cerr << "lonDeg=" << lonDeg
  //           << " latDeg=" << latDeg
  //           << " x=" << src[0]
  //           << " xp=" << v[0]
  //           << " yp=" << v[1]
  //           << " zp=" << v[2] << endl;
}

//------------------------------------------------------------------------------
// We map a point 'v' on a sphere centered at (0, 0, 0) to the
// intersection between the ray from the center to 'v' with the axis
// aligned cube that has the sphere as circumscribed sphere.
void rayBoxIntersection(double r, double v[3])
{
  // The face order is X, -X, Y, -Y, Z, -Z
  double faceNormal[NFACES][3] = {
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 },
  };
  double side = r * 2 / sqrt3;
  double minT = std::numeric_limits<double>::max();
  for (int i = 0; i < NFACES; ++i)
  {
    double t = (side / 2) / vtkMath::Dot(v, faceNormal[i]);
    if (t > 0 && t < minT)
    {
      minT = t;
    }
  }
  vtkMath::MultiplyScalar(v, minT);
}
} // anonymous namespace

namespace CamAdaptor
{
//==============================================================================
template <GridType gridType>
class Grid<gridType>::Internals
{
public:
  Internals() {}
  ~Internals() {}
  typedef std::map<size_t, vtkIdType> MapType;
  std::map<size_t, vtkIdType> CellId2d;  // lon x lat -> cellId
  std::map<size_t, vtkIdType> PointId2d; // lon x lat -> pointId
  std::map<size_t, vtkIdType> CellId3d;  // lon x lat x lev -> cellId
  std::map<size_t, vtkIdType> PointId3d; // lon x lat x lev -> pointId
  //////////////////////////////////////////////////////////////////////
  // SE dyngrid only
  std::vector<double> CubeCoordinates;
  //////////////////////////////////////////////////////////////////////
};

//------------------------------------------------------------------------------
template <GridType gridType>
Grid<gridType>::Grid()
  : MpiRank(-1)
  , ChunkCapacity(0)
  , NCells2d(0)
  , NLon(0)
  , NLat(0)
  , NLev(0)
  , Lev(NULL)
  , Radius(0)
  , Ne(0)
  , Np(0)
  , LonStep(0)
  , LatStep(0)
  , Impl(new Internals())
  , RankArrayIndex2d(-1)
  , CoordArrayIndex2d(-1)
  , PSArrayIndex(-1)
  , RankArrayIndex3d(-1)
  , CoordArrayIndex3d(-1)
  , TArrayIndex(-1)
  , UArrayIndex(-1)
  , VArrayIndex(-1)
{
}

//------------------------------------------------------------------------------
template <GridType gridType>
Grid<gridType>::~Grid()
{
  if (this->Lev)
  {
    delete[] this->Lev;
  }
  delete this->Impl;
}

//------------------------------------------------------------------------------
// Creates a 2D and a 3D grid of the specified 'type'
template <GridType gridType>
void Grid<gridType>::Create()
{
  this->Grid2d = CreateGrid2d();
  this->Grid3d = CreateGrid3d();
}

//------------------------------------------------------------------------------
// Creates the 2D grid and the data used to add attributes to the grid
template <GridType gridType>
vtkSmartPointer<vtkUnstructuredGrid> Grid<gridType>::CreateGrid2d()
{
  vtkSmartPointer<vtkUnstructuredGrid> grid2d = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkSmartPointer<vtkPoints> points2d = vtkSmartPointer<vtkPoints>::New();
  grid2d->SetPoints(points2d);
  grid2d->GetCells()->Initialize();
  grid2d->Allocate(this->NCells2d);
  this->RankArrayIndex2d = addAttribute<vtkIntArray>(grid2d, "Rank", this->NCells2d, 1);
  this->CoordArrayIndex2d = addAttribute<vtkDoubleArray>(grid2d, "Coord", this->NCells2d, 2);
  this->PSArrayIndex = addAttribute<vtkDoubleArray>(grid2d, "PS", this->NCells2d, 1);
  // the pointer is managed by vtkCPInputDataDescription
  return grid2d;
}

//------------------------------------------------------------------------------
// Creates the 3D grid and the data used to add attributes to the grid
template <GridType gridType>
vtkSmartPointer<vtkUnstructuredGrid> Grid<gridType>::CreateGrid3d()
{
  vtkSmartPointer<vtkUnstructuredGrid> grid3d = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkSmartPointer<vtkPoints> points3d = vtkSmartPointer<vtkPoints>::New();
  grid3d->SetPoints(points3d);
  grid3d->GetCells()->Initialize();
  grid3d->Allocate(this->NCells2d * this->NLev);
  this->RankArrayIndex3d =
    addAttribute<vtkIntArray>(grid3d, "Rank", this->NCells2d * this->NLev, 1);
  this->CoordArrayIndex3d =
    addAttribute<vtkDoubleArray>(grid3d, "Coord", this->NCells2d * this->NLev, 3);
  this->TArrayIndex = addAttribute<vtkDoubleArray>(grid3d, "T", this->NCells2d * this->NLev, 1);
  this->UArrayIndex = addAttribute<vtkDoubleArray>(grid3d, "U", this->NCells2d * this->NLev, 1);
  this->VArrayIndex = addAttribute<vtkDoubleArray>(grid3d, "V", this->NCells2d * this->NLev, 1);
  return grid3d;
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::AddPointsAndCells(double lonRad, double latRad)
{
  std::ostringstream ostr;

  Internals* impl = this->Impl;
  // SE dynamic grid comment:
  // for points shared between 2 or 3 faces we still need a different index
  // for each shared face, because we need to create a different cell for
  // each face. For FV grid the vector always has one element.
  std::vector<std::vector<int> > indexArray;
  this->GetValueIndex(lonRad, latRad, indexArray);
  // ostr << "AddPointsAndCells: " << vtkMath::DegreesFromRadians(lonRad)
  //      << ", " << vtkMath::DegreesFromRadians(latRad) << " indexArraySize: "
  //      << indexArray.size() << std::endl;
  // std::cerr << ostr.str();
  for (size_t i = 0; i < indexArray.size(); ++i)
  {
    // ostr.str("");
    // ostr << "After for index" << std::endl;
    // std::cerr << ostr.str();
    int* index = &indexArray[i][0];
    // 2d grid
    {
      int pointIds[4];
      this->GetPointIds(index, pointIds);
      double cellPoints[4][3];
      this->GetCellPoints(index, cellPoints);
      vtkIdType realPointIds[4];
      std::fill(realPointIds, realPointIds + 4, -1);
      for (int p = 0; p < 4; ++p)
      {
        int pointId = pointIds[p];
        vtkIdType realId = -1;
        typename Internals::MapType::iterator it = impl->PointId2d.find(pointId);
        if (it == impl->PointId2d.end())
        {
          realId = impl->PointId2d[pointId] = this->Grid2d->GetPoints()->InsertNextPoint(
            cellPoints[p][0], cellPoints[p][1], cellPoints[p][2]);
          // ostr.str("");
          // ostr << "Insert new point id: "
          //      << pointId << " index: "
          //      << index[0] << ", " << index[1] << ", " << index[2]
          //      << " lonDeg: " << vtkMath::DegreesFromRadians(lonRad)
          //      << " latDeg: " << vtkMath::DegreesFromRadians(latRad)
          //      << std::endl;
          // std::cerr << ostr.str();
        }
        else
        {
          realId = it->second;
          // ostr.str("");
          // ostr << "Found point id: "
          //      << pointId << " index: "
          //      << index[0] << ", " << index[1] << ", " << index[2]
          //      << " lonDeg: " << vtkMath::DegreesFromRadians(lonRad)
          //      << " latDeg: " << vtkMath::DegreesFromRadians(latRad)
          //      << std::endl;
          // std::cerr << ostr.str();
        }
        realPointIds[p] = realId;
      }
      vtkIdType realCellId = this->Grid2d->InsertNextCell(VTK_QUAD, 4, realPointIds);
      assert(impl->CellId2d.find(this->GetCellId(index)) == impl->CellId2d.end());
      impl->CellId2d[this->GetCellId(index)] = realCellId;
    }
    // ostr.str("");
    // ostr << "Create cell id: " << cellId << " real id: " << realCellId
    //      << std::endl;
    // std::cerr << ostr.str();

    // 3d grid
    // ostr.str("");
    for (int level = 0; level < this->NLev; ++level)
    {
      int pointIds[8];
      this->GetPointIds(index, level, pointIds);
      double corner[8][3];
      this->GetCellPoints(index, level, corner);
      vtkIdType realPointIds[8];
      for (int p = 0; p < 8; ++p)
      {
        int pointId = pointIds[p];
        vtkIdType realId = -1;
        typename Internals::MapType::iterator it = impl->PointId3d.find(pointId);
        if (it == impl->PointId3d.end())
        {
          realId = impl->PointId3d[pointId] =
            this->Grid3d->GetPoints()->InsertNextPoint(corner[p][0], corner[p][1], corner[p][2]);
          // std::cerr << "InsertNextPoint("
          //           << corner[p][0] << ", " << corner[p][1] << ", "
          //           << corner[p][2] << ")"
          //           << "\npointId=" << pointId << endl;
        }
        else
        {
          realId = it->second;
        }
        realPointIds[p] = realId;
      }
      vtkIdType realCellId = this->Grid3d->InsertNextCell(VTK_HEXAHEDRON, 8, realPointIds);
      int cellId = this->GetCellId(index, level);
      impl->CellId3d[cellId] = realCellId;
      ostr << "Create 3D cell id: " << cellId << " real id: " << realCellId << std::endl;
    }
    // std::cerr << ostr.str();
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::SetAttributeValue(int chunkSize, double* lonRad, double* latRad,
  double* psScalar, double* tScalar, double* uScalar, double* vScalar)
{
  Internals* impl = this->Impl;
  std::ostringstream ostr;
  for (int c = 0; c < chunkSize; ++c)
  {
    double lonDeg = vtkMath::DegreesFromRadians(lonRad[c]);
    double latDeg = vtkMath::DegreesFromRadians(latRad[c]);
    std::vector<std::vector<int> > indexArray;
    this->GetValueIndex(lonRad[c], latRad[c], indexArray);
    for (size_t i = 0; i < indexArray.size(); ++i)
    {
      int* index = &indexArray[i][0];
      // 2d attributes
      {
        int cellId = this->GetCellId(index);
        typename Internals::MapType::iterator cellIt = impl->CellId2d.find(cellId);
        if (cellIt == impl->CellId2d.end())
        {
          vtkGenericWarningMacro(<< "Invalid 2D cell at: " << index[0] << ", " << index[1] << endl);
          exit(13);
        }
        vtkIdType realCellId = cellIt->second;
        vtkDoubleArray::SafeDownCast(this->Grid2d->GetCellData()->GetArray(this->PSArrayIndex))
          ->SetValue(realCellId, psScalar[c]);
        this->Grid2d->GetCellData()
          ->GetArray(this->CoordArrayIndex2d)
          ->SetTuple2(realCellId, lonDeg, latDeg);
        vtkIntArray::SafeDownCast(this->Grid2d->GetCellData()->GetArray(this->RankArrayIndex2d))
          ->SetValue(realCellId, this->MpiRank);
      }
      // ostr.str("");
      // ostr << "Set cell value cell id: " << cellId << " real id: " << realCellId
      //      << std::endl;
      // std::cerr << ostr.str();

      // 3d attributes
      // ostr.str("");
      for (int level = 0; level < this->NLev; ++level)
      {
        int cellId = this->GetCellId(index, level);
        typename Internals::MapType::iterator cellIt = impl->CellId3d.find(cellId);
        if (cellIt == impl->CellId3d.end())
        {
          vtkGenericWarningMacro(<< "Invalid 3D cell at: " << index[0] << ", " << index[1] << ", "
                                 << level << endl);
          exit(13);
        }
        vtkIdType realCellId = cellIt->second;
        ostr << "Set 3D cell value cell id: " << cellId << " real id: " << realCellId << std::endl;

        vtkIntArray::SafeDownCast(this->Grid3d->GetCellData()->GetArray(this->RankArrayIndex3d))
          ->SetValue(realCellId, this->MpiRank);
        vtkDoubleArray::SafeDownCast(this->Grid3d->GetCellData()->GetArray(this->TArrayIndex))
          ->SetValue(realCellId, tScalar[c + level * this->ChunkCapacity]);
        vtkDoubleArray::SafeDownCast(this->Grid3d->GetCellData()->GetArray(this->UArrayIndex))
          ->SetValue(realCellId, uScalar[c + level * this->ChunkCapacity]);
        vtkDoubleArray::SafeDownCast(this->Grid3d->GetCellData()->GetArray(this->VArrayIndex))
          ->SetValue(realCellId, vScalar[c + level * this->ChunkCapacity]);
        this->Grid3d->GetCellData()
          ->GetArray(this->CoordArrayIndex3d)
          ->SetTuple3(realCellId, lonDeg, latDeg, this->Lev[level]);
      }
      // std::cerr << ostr.str();
    }
  }
}

//------------------------------------------------------------------------------
// Print routine for debugging
template <GridType gridType>
void Grid<gridType>::PrintAddChunk(
  int* nstep, int* chunkCols, double* lonRad, double* latRad, double* psScalar, double* tScalar)
{
  std::ostringstream ostr;
  ostr << "add_chunk: " << *nstep << "\nchunkCols: " << *chunkCols
       << "\nnCells: " << this->Grid2d->GetNumberOfCells()
       << "\nnPoints: " << this->Grid2d->GetPoints()->GetNumberOfPoints()
       << "\nnCellArrays: " << this->Grid2d->GetCellData()->GetNumberOfArrays()
       << "\nmyRank: " << this->MpiRank << "\nlon: ";
  for (int c = 0; c < *chunkCols; ++c)
  {
    ostr << vtkMath::DegreesFromRadians(lonRad[c]) << " ";
  }
  ostr << "\nlat: ";
  for (int c = 0; c < *chunkCols; ++c)
  {
    ostr << vtkMath::DegreesFromRadians(latRad[c]) << " ";
  }
  ostr << "\nPS: ";
  for (int c = 0; c < *chunkCols; ++c)
  {
    ostr << psScalar[c] << " ";
  }
  ostr << "\nt: ";
  for (int c = 0; c < 2 * (*chunkCols); ++c)
  {
    ostr << tScalar[c] << " ";
  }
  ostr << endl;
  std::cerr << ostr.str();
}

//------------------------------------------------------------------------------
template <GridType gridType>
double Grid<gridType>::LevelToRadius(double level) const
{
  double maxLevel = this->Lev[this->NLev - 1];
  if (gridType == SPHERE || gridType == CUBE_SPHERE)
  {
    return (maxLevel - level) / maxLevel;
  }
  else
  {
    return -level;
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetLevMinusPlus(int j, double* levMinus, double* levPlus) const
{
  if (j == 0)
  {
    double step = (this->Lev[1] - this->Lev[0]);
    *levMinus = this->Lev[j];
    *levPlus = this->Lev[j] + step / 2;
  }
  else if (j == this->NLev - 1)
  {
    double step = (this->Lev[this->NLev - 1] - this->Lev[this->NLev - 2]);
    *levMinus = this->Lev[j] - step / 2;
    *levPlus = this->Lev[j];
  }
  else
  {
    *levMinus = this->Lev[j] - (this->Lev[j] - this->Lev[j - 1]) / 2;
    *levPlus = this->Lev[j] + (this->Lev[j + 1] - this->Lev[j]) / 2;
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetLatMinusPlus(int latIndex, double* latMinus, double* latPlus) const
{
  double latDeg = -90 + this->LatStep * latIndex;
  if (latIndex == 0)
  {
    *latMinus = latDeg;
    *latPlus = latDeg + this->LatStep / 2;
  }
  else if (latIndex == this->NLat - 1)
  {
    *latMinus = latDeg - this->LatStep / 2;
    *latPlus = latDeg;
  }
  else
  {
    *latMinus = latDeg - this->LatStep / 2;
    *latPlus = latDeg + this->LatStep / 2;
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetLonMinusPlus(int lonIndex, double* lonMinus, double* lonPlus) const
{
  double lonDeg = this->LonStep * lonIndex;
  *lonMinus = lonDeg - this->LonStep / 2;
  *lonPlus = lonDeg + this->LonStep / 2;
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetCellPoints(const int index[3], double cell[4][3]) const
{
  Internals* impl = this->Impl;
  if (gridType == CUBE_SPHERE)
  {
    // index space
    double cellIndex[4][3];
    cellIndex[0][0] = cellIndex[1][0] = (index[0] > 0) ? index[0] - 0.5 : index[0];
    cellIndex[2][0] = cellIndex[3][0] = (index[0] < this->NLon - 1) ? index[0] + 0.5 : index[0];
    cellIndex[0][1] = cellIndex[3][1] = (index[1] > 0) ? index[1] - 0.5 : index[1];
    cellIndex[1][1] = cellIndex[2][1] = (index[1] < this->NLon - 1) ? index[1] + 0.5 : index[1];
    cellIndex[0][2] = cellIndex[1][2] = cellIndex[2][2] = cellIndex[3][2] = index[2];
    // std::ostringstream ostr;
    // ostr << "cell points - index space: index: " << index[0] << ", " << index[1] << ", " <<
    // index[2]
    //      << " cell0: " << cellIndex[0][0] << ", " << cellIndex[0][1] << ", " << cellIndex[0][2]
    //      << " cell1: " << cellIndex[1][0] << ", " << cellIndex[1][1] << ", " << cellIndex[1][2]
    //      << " cell2: " << cellIndex[2][0] << ", " << cellIndex[2][1] << ", " << cellIndex[2][2]
    //      << " cell3: " << cellIndex[3][0] << ", " << cellIndex[3][1] << ", " << cellIndex[3][2]
    //      << std::endl;
    // std::cerr << ostr.str();
    for (int i = 0; i < 4; ++i)
    {
      // cube space
      this->IndexToCube(impl->CubeCoordinates, cellIndex[i], cell[i]);
      // ostr.str("");
      // ostr << "cell points - cube space: index: " << index[0] << ", " << index[1] << ", " <<
      // index[2]
      //      << " cell"  << i << ": "<< cell[i][0] << ", " << cell[i][1] << ", " << cell[i][2]
      //      << std::endl;
      // std::cerr << ostr.str();

      // spherical space
      double lonRad, latRad;
      this->CubeToSpherical(cell[i], &lonRad, &latRad);
      // ostr.str("");
      // ostr << "cell points - spherical: index: " << index[0] << ", " << index[1] << ", " <<
      // index[2]
      //      << " cell"  << i << ": "<< vtkMath::DegreesFromRadians(lonRad)
      //      << ", " << vtkMath::DegreesFromRadians(latRad)
      //      << std::endl;
      // std::cerr << ostr.str();
      cell[i][0] = lonRad;
      cell[i][1] = latRad;
      cell[i][2] = 1;
      // Cartesian space
      sphericalToCartesian(cell[i]);
    }
  }
  else
  {
    double lonMinus, lonPlus;
    this->GetLonMinusPlus(index[0], &lonMinus, &lonPlus);
    double latMinus, latPlus;
    this->GetLatMinusPlus(index[1], &latMinus, &latPlus);
    double corner[4][3] = { { lonMinus, latMinus, 1 }, { lonMinus, latPlus, 1 },
      { lonPlus, latPlus, 1 }, { lonPlus, latMinus, 1 } };
    if (gridType == SPHERE)
    {
      for (int i = 0; i < 4; ++i)
      {
        sphericalDegToCartesian(corner[i]);
      }
    }
    memcpy(cell, corner, 4 * 3 * sizeof(double));
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetCellPoints(const int index[3], int level, double cell[8][3]) const
{
  Internals* impl = this->Impl;
  double levMinus, levPlus;
  this->GetLevMinusPlus(level, &levMinus, &levPlus);
  levMinus = this->LevelToRadius(levMinus);
  levPlus = this->LevelToRadius(levPlus);
  if (gridType == CUBE_SPHERE)
  {
    // index space
    double cellIndex[8][3];
    cellIndex[0][0] = cellIndex[3][0] = cellIndex[4][0] = cellIndex[7][0] =
      (index[0] > 0) ? index[0] - 0.5 : index[0];
    cellIndex[1][0] = cellIndex[2][0] = cellIndex[5][0] = cellIndex[6][0] =
      (index[0] < this->NLon - 1) ? index[0] + 0.5 : index[0];
    cellIndex[0][1] = cellIndex[1][1] = cellIndex[4][1] = cellIndex[5][1] =
      (index[1] > 0) ? index[1] - 0.5 : index[1];
    cellIndex[2][1] = cellIndex[3][1] = cellIndex[6][1] = cellIndex[7][1] =
      (index[1] < this->NLon - 1) ? index[1] + 0.5 : index[1];
    cellIndex[0][2] = cellIndex[1][2] = cellIndex[2][2] = cellIndex[3][2] = cellIndex[4][2] =
      cellIndex[5][2] = cellIndex[6][2] = cellIndex[7][2] = index[2];
    // std::ostringstream ostr;
    // ostr << "cell points 3D - index space: index: " << index[0] << ", " << index[1] << ", " <<
    // index[2]
    //      << " cell0: " << cellIndex[0][0] << ", " << cellIndex[0][1] << ", " << cellIndex[0][2]
    //      << " cell1: " << cellIndex[1][0] << ", " << cellIndex[1][1] << ", " << cellIndex[1][2]
    //      << " cell2: " << cellIndex[2][0] << ", " << cellIndex[2][1] << ", " << cellIndex[2][2]
    //      << " cell3: " << cellIndex[3][0] << ", " << cellIndex[3][1] << ", " << cellIndex[3][2]
    //      << std::endl;
    // std::cerr << ostr.str();
    double levels[8] = { levMinus, levMinus, levMinus, levMinus, levPlus, levPlus, levPlus,
      levPlus };
    for (int i = 0; i < 8; ++i)
    {
      // cube space
      this->IndexToCube(impl->CubeCoordinates, cellIndex[i], cell[i]);
      // ostr.str("");
      // ostr << "cell points 3D - cube space: index: " << index[0] << ", " << index[1] << ", " <<
      // index[2]
      //      << " cell"  << i << ": "<< cell[i][0] << ", " << cell[i][1] << ", " << cell[i][2]
      //      << std::endl;
      // std::cerr << ostr.str();

      // spherical space
      double lonRad, latRad;
      this->CubeToSpherical(cell[i], &lonRad, &latRad);
      // ostr.str("");
      // ostr << "cell points 3D - spherical: index: " << index[0] << ", " << index[1] << ", " <<
      // index[2]
      //      << " cell"  << i << ": "<< vtkMath::DegreesFromRadians(lonRad)
      //      << ", " << vtkMath::DegreesFromRadians(latRad)
      //      << std::endl;
      // std::cerr << ostr.str();
      cell[i][0] = lonRad;
      cell[i][1] = latRad;
      cell[i][2] = levels[i];
      // Cartesian space
      sphericalToCartesian(cell[i]);
    }
  }
  else
  {
    double lonMinus, lonPlus;
    this->GetLonMinusPlus(index[0], &lonMinus, &lonPlus);
    double latMinus, latPlus;
    this->GetLatMinusPlus(index[1], &latMinus, &latPlus);

    double corner[8][3] = { { lonMinus, latMinus, levMinus }, { lonPlus, latMinus, levMinus },
      { lonPlus, latPlus, levMinus }, { lonMinus, latPlus, levMinus },
      { lonMinus, latMinus, levPlus }, { lonPlus, latMinus, levPlus },
      { lonPlus, latPlus, levPlus }, { lonMinus, latPlus, levPlus } };
    if (gridType == SPHERE)
    {
      for (int i = 0; i < 8; ++i)
      {
        sphericalDegToCartesian(corner[i]);
      }
    }
    memcpy(cell, corner, 8 * 3 * sizeof(double));
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
int Grid<gridType>::GetCellId(const int index[3]) const
{
  if (gridType == CUBE_SPHERE)
  {
    return index[0] + index[1] * this->NLon + index[2] * (this->NLon * this->NLat);
  }
  else
  {
    int lonIndex = index[0], latIndex = index[1];
    return lonIndex + latIndex * this->NLon;
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
int Grid<gridType>::GetCellId(const int index[3], int level) const
{
  if (gridType == CUBE_SPHERE)
  {
    int s = index[0], t = index[1], face = index[2];
    return s + t * this->NLon + face * this->NLon * this->NLat +
      level * (this->NLon * this->NLat * NFACES);
  }
  else
  {
    int lonIndex = index[0], latIndex = index[1];
    return lonIndex + latIndex * this->NLon + level * this->NLon * this->NLat;
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetPointIds(const int index[3], int pointIds[4]) const
{
  int s = index[0], t = index[1];
  int temp[4][2] = { { s, t }, { s, t + 1 }, { s + 1, t + 1 }, { s + 1, t } };
  if (gridType == CUBE_SPHERE)
  {
    for (int p = 0; p < 4; ++p)
    {
      int tempPointId[3] = { temp[p][0], temp[p][1], index[2] };
      this->MinFaceIndex(this->NLon, tempPointId);
      pointIds[p] = tempPointId[0] + tempPointId[1] * (this->NLon + 1) +
        tempPointId[2] * ((this->NLon + 1) * (this->NLon + 1));
    }
  }
  else
  {
    for (int p = 0; p < 4; ++p)
    {
      pointIds[p] = temp[p][0] + temp[p][1] * (this->NLon + 1);
    }
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetPointIds(const int index[3], int level, int pointIds[8]) const
{
  int s = index[0], t = index[1];
  int temp[8][3] = { { s, t, level }, { s + 1, t, level }, { s + 1, t + 1, level },
    { s, t + 1, level }, { s, t, level + 1 }, { s + 1, t, level + 1 }, { s + 1, t + 1, level + 1 },
    { s, t + 1, level + 1 } };
  if (gridType == CUBE_SPHERE)
  {
    for (int p = 0; p < 8; ++p)
    {
      int tempPointId[3] = { temp[p][0], temp[p][1], index[2] };
      this->MinFaceIndex(this->NLon, tempPointId);
      pointIds[p] = tempPointId[0] + tempPointId[1] * (this->NLon + 1) +
        tempPointId[2] * ((this->NLon + 1) * (this->NLat + 1)) +
        temp[p][2] * ((this->NLon + 1) * (this->NLat + 1) * NFACES);
    }
  }
  else
  {
    for (int p = 0; p < 8; ++p)
    {
      pointIds[p] = temp[p][0] + temp[p][1] * (this->NLon + 1) +
        temp[p][2] * (this->NLon + 1) * (this->NLat + 1);
    }
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::MinFaceIndex(int side, int index[3]) const
{
  // convert to cube coordinates
  double indexD[3] = { static_cast<double>(index[0]), static_cast<double>(index[1]),
    static_cast<double>(index[2]) };
  double sideD = side;
  std::vector<double> cubeCoordinates(side + 1);
  double value = -sideD / 2;
  for (int i = 0; i <= side; ++i)
  {
    cubeCoordinates[i] = value;
    value += 1.0;
  }
  double v[3];
  this->IndexToCube(cubeCoordinates, indexD, v);
  // std::ostringstream ostr;
  // ostr << "MinFaceIndex index: "
  //      << index[0] << ", " << index[1] << ", " << index[2];
  std::vector<std::vector<int> > allIndexes;
  size_t mfi = this->CubeToIndex(sideD, cubeCoordinates, v, allIndexes);
  std::copy(allIndexes[mfi].begin(), allIndexes[mfi].end(), index);
  // ostr << " result: "
  //      << index[0] << ", " << index[1] << ", " << index[2]
  //      << std::endl;
  // std::cerr << ostr.str();
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::GetValueIndex(
  double lonRad, double latRad, std::vector<std::vector<int> >& index) const
{
  Internals* impl = this->Impl;
  if (gridType == CUBE_SPHERE)
  {
    double v[3] = { lonRad, latRad, this->Radius };
    // std::ostringstream ostr;
    // ostr << " Lon: " << vtkMath::DegreesFromRadians(lonRad)
    //      << " Lat: " << vtkMath::DegreesFromRadians(latRad)
    //      << std::endl;
    sphericalToCartesian(v);
    // ostr << " Sphere intersection: "
    //      << v[0] << ", " << v[1] << ", " << v[2]
    //      << std::endl;
    rayBoxIntersection(this->Radius, v);
    this->CubeToIndex(this->NLon - 1, impl->CubeCoordinates, v, index);
    // ostr << " Cube intersection: "
    //      << v[0] << ", " << v[1] << ", " << v[2] << std::endl;
    // ostr << " Cube index(" << index.size() << "): ";
    // for (int i = 0; i < index.size(); ++i)
    //   {
    //   ostr << index[i][0] << ", " << index[i][1] << ", " << index[i][2]
    //        << "    ";
    //   }
    // ostr << std::endl;
    // std::cerr << ostr.str();
  }
  else
  {
    double lonDeg = vtkMath::DegreesFromRadians(lonRad);
    double latDeg = vtkMath::DegreesFromRadians(latRad);
    int oneIndex[3] = {
      static_cast<int>(round(lonDeg / this->LonStep)),        // interval [0, 360]
      static_cast<int>(round((90 + latDeg) / this->LatStep)), // interval [-90, 90]
      0                                                       // not used
    };
    index.resize(1);
    index[0].resize(3);
    std::copy(oneIndex, oneIndex + 3, index[0].begin());
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
bool Grid<gridType>::SetToCoprocessor(vtkCPDataDescription* coprocessorData, const char* name,
  vtkSmartPointer<vtkUnstructuredGrid> grid)
{
  vtkCPInputDataDescription* idd = coprocessorData->GetInputDescriptionByName(name);
  if (idd)
  {
    idd->SetGrid(grid);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::SetCubeGridPoints(
  int ne, int np, int lonSize, double* lonRad, int latSize, double* latRad)
{
  (void)latSize;
  Internals* impl = this->Impl;
  assert(gridType == CUBE_SPHERE);
  this->NLon = ne * (np - 1) + 1;
  this->NLat = ne * (np - 1) + 1;
  this->Ne = ne;
  this->Np = np;
  this->Radius = (ne * (np - 1)) / 2 * sqrt3;
  std::cerr << "Computing the grid coordinates ...\n";
  std::set<double, FuzzyLT1E_10> cubeCoordinates;
  for (int i = 0; i < lonSize; ++i)
  {
    double v[3] = { lonRad[i], latRad[i], this->Radius };
    sphericalToCartesian(v);
    rayBoxIntersection(this->Radius, v);
    cubeCoordinates.insert(v[0]);
  }
  impl->CubeCoordinates.resize(cubeCoordinates.size());
  std::copy(cubeCoordinates.begin(), cubeCoordinates.end(), impl->CubeCoordinates.begin());
  if (this->MpiRank == 0)
  {
    std::ofstream of("c-x.txt");
    std::ostringstream ostr;
    for (size_t i = 0; i < impl->CubeCoordinates.size(); ++i)
    {

      ostr << std::setprecision(std::numeric_limits<double>::digits10) << impl->CubeCoordinates[i]
           << std::endl;
    }
    of << ostr.str();
  }
  assert(impl->CubeCoordinates.size() == static_cast<size_t>(this->NLon));
}

//------------------------------------------------------------------------------
template <GridType gridType>
size_t Grid<gridType>::CubeToIndex(double side, const std::vector<double>& cubeCoordinates,
  double v[3], std::vector<std::vector<int> >& index) const
{
  FuzzyEQ eq(MAX_DIFF);
  // The face order is X, -X, Y, -Y, Z, -Z
  // (lon,lat) on a face are chosen such that the right hand rule
  // points to the positive axis for the face
  struct
  {
    double value;
    int index[3]; // 0: index to test for equality with value, 1,2: lon and lat
    int face;
  } valIndex[NFACES] = { { side / 2, { 0, 1, 2 }, 0 }, { -side / 2, { 0, 1, 2 }, 1 },
    { side / 2, { 1, 2, 0 }, 2 }, { -side / 2, { 1, 2, 0 }, 3 }, { side / 2, { 2, 0, 1 }, 4 },
    { -side / 2, { 2, 0, 1 }, 5 } };
  int minFace = std::numeric_limits<int>::max();
  size_t minFaceIndex = std::numeric_limits<size_t>::max();
  for (int i = 0; i < NFACES; ++i)
  {
    if (eq(v[valIndex[i].index[0]], valIndex[i].value))
    {
      std::vector<double>::const_iterator it1 = lower_bound(
        cubeCoordinates.begin(), cubeCoordinates.end(), v[valIndex[i].index[1]], FuzzyLT1E_10());
      std::vector<double>::const_iterator it2 = lower_bound(
        cubeCoordinates.begin(), cubeCoordinates.end(), v[valIndex[i].index[2]], FuzzyLT1E_10());
      assert(it1 != cubeCoordinates.end() && it2 != cubeCoordinates.end());
      int oneIndex[3] = { static_cast<int>(it1 - cubeCoordinates.begin()),
        static_cast<int>(it2 - cubeCoordinates.begin()), valIndex[i].face };
      index.push_back(std::vector<int>(oneIndex, oneIndex + 3));
      if (valIndex[i].face < minFace)
      {
        minFace = valIndex[i].face;
        minFaceIndex = index.size() - 1;
      }
    }
  }
  assert(index.size() >= 1 && index.size() <= 3);
  return minFaceIndex;
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::IndexToCube(
  const std::vector<double>& cubeCoordinates, double index[3], double v[3]) const
{
  FuzzyEQ eq(MAX_DIFF);
  // The face order is X, -X, Y, -Y, Z, -Z
  // (lon,lat) on  a face  are chosen  such that  the right  hand rule
  // points to the axis for the face

  // 2: where does face value go, 0,1: where do lon,lat go.
  int position[NFACES][3] = {
    { 1, 2, 0 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 0, 1 }, { 0, 1, 2 }, { 0, 1, 2 },
  };
  double faceValue[NFACES] = {
    cubeCoordinates[cubeCoordinates.size() - 1], cubeCoordinates[0],
    cubeCoordinates[cubeCoordinates.size() - 1], cubeCoordinates[0],
    cubeCoordinates[cubeCoordinates.size() - 1], cubeCoordinates[0],
  };
  int face = index[2];
  v[position[face][2]] = faceValue[face];
  for (int i = 0; i < 2; ++i)
  {
    int floorIndex = floor(index[i]);
    if (eq(floorIndex, cubeCoordinates.size() - 1))
    {
      v[position[face][i]] = cubeCoordinates[floorIndex];
    }
    else
    {
      double t = index[i] - floorIndex;
      // average the two neighboring positions
      v[position[face][i]] =
        cubeCoordinates[floorIndex] * (1 - t) + cubeCoordinates[floorIndex + 1] * t;
    }
  }
}

//------------------------------------------------------------------------------
template <GridType gridType>
void Grid<gridType>::CubeToSpherical(double v[3], double* lonRad, double* latRad) const
{
  *latRad = atan(v[2] / sqrt(v[0] * v[0] + v[1] * v[1]));
  *lonRad = atan2(v[1], v[0]) + PI;
}

// instantiations for the template classes
template class Grid<RECTILINEAR>;
template class Grid<SPHERE>;
template class Grid<CUBE_SPHERE>;
} // namespace CamAdaptor
