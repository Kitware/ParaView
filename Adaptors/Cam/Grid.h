/*=========================================================================

  Program:   ParaView
  Module:    Grid.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __GRID_H__
#define __GRID_H__

#include "vtkPVAdaptorsCamModule.h"
#include "vtkSmartPointer.h" // For smart pointers

#include <string> // for string
#include <vector> // for vector

class vtkCPDataDescription;
class vtkDataObject;
class vtkUnstructuredGrid;

namespace CamAdaptor
{

enum GridType
{
  RECTILINEAR,
  SPHERE,
  CUBE_SPHERE,
  GRID_TYPE_COUNT
};

/// Creates and accumulates data for a 2D and a 3D grid. It generates either
/// a parallelepiped or sphere for the Finite Volume (FV) dynamic core
/// or a cube sphere for the Structured Element (SE) dynamic core.
/// Grids are stored as unstructured grids, and are reconstructed from points.
template <GridType gridType>
class VTKPVADAPTORSCAM_EXPORT Grid
{
public:
  Grid();
  /// Deletes data used to build the grids. Note that the grid memory
  /// is managed by the Catalyst Coprocessor.
  ~Grid();

  /// Creates a 2D and a 3D grid
  void Create();
  void SetMpiRank(int rank) { this->MpiRank = rank; }
  void SetChunkCapacity(int capacity) { this->ChunkCapacity = capacity; }
  void SetNCells2d(int ncells) { this->NCells2d = ncells; }
  void SetNLon(int nlon) { this->NLon = nlon; }
  void SetNLat(int nlat) { this->NLat = nlat; }
  void SetLev(int nlev, double* lev)
  {
    this->NLev = nlev;
    if (lev)
    {
      this->Lev = new double[nlev];
      std::copy(lev, lev + nlev, this->Lev);
    }
  }
  /// Adds the points and the cells for a vertical column to the grid
  void AddPointsAndCells(double lonRad, double latRad);

  /// Sets attributes for a chunk (a list of vertical columns) to
  /// the 2D and 3D grids
  void SetAttributeValue(int chunkSize, double* lonRad, double* latRad, double* psScalar,
    double* tScalar, double* uScalar, double* vScalar);

  /// Returns the 2D grid
  vtkSmartPointer<vtkUnstructuredGrid> GetGrid2d() const { return this->Grid2d; }
  /// Returns the 3D grid
  vtkSmartPointer<vtkUnstructuredGrid> GetGrid3d() const { return this->Grid3d; }
  /// Attach the grid to the coprocessor data
  static bool SetToCoprocessor(vtkCPDataDescription* coprocessorData, const char* name,
    vtkSmartPointer<vtkUnstructuredGrid> grid);

  ///////////////////////////////////////////////////////////////////////////
  /// used for the FV dynamic core only
  void SetLonStep(int step) { this->LonStep = step; }
  void SetLatStep(int step) { this->LatStep = step; }
  /// used for the SE dynamic core only
  void SetCubeGridPoints(int ne, int np, int lonSize, double* lonRad, int latSize, double* latRad);
  ///////////////////////////////////////////////////////////////////////////

private:
  /// Print routine for debugging
  void PrintAddChunk(
    int* nstep, int* chunkCols, double* lonRad, double* latRad, double* psScalar, double* tScalar);

  /// Creates the 2D grid and the data used to add attributes to the grid
  vtkSmartPointer<vtkUnstructuredGrid> CreateGrid2d();

  /// Creates the 3D grid and the data used to add attributes to the grid
  vtkSmartPointer<vtkUnstructuredGrid> CreateGrid3d();

  /// The level for a grid measures preasure which decreases with height.
  /// This transformation sets
  /// - max pressure to 0 (surface level) and min pressure to 1 (SPHERE)
  /// - negates the pressure so that globe surface (high pressure) is closer to
  ///   origin than high in the atmosphere (low pressure) (RECTILINEAR)
  /// These transformations match the transformation in the history file.
  double LevelToRadius(double level) const;

  /// Compute the  location of  the points surrounding  a cell  at index
  /// 'levIndex', 'latIndex' or 'lonIndex'
  void GetLevMinusPlus(int levIndex, double* levMinus, double* levPlus) const;
  void GetLatMinusPlus(int latIndex, double* latMinus, double* latPlus) const;
  void GetLonMinusPlus(int lonIndex, double* lonMinus, double* lonPlus) const;

  /// Returns (lonIndex, latIndex) for FV dynamic core (regular grid) or
  /// (s, t, face) for SE dynamic core (cube sphere grid).
  void GetValueIndex(double lonRad, double latRad, std::vector<std::vector<int> >& index) const;

  /// Points for a 2D cell surrounding value at index for 2D or (index, level) for 3D.
  void GetCellPoints(const int index[3], double cellPoints[4][3]) const;
  void GetCellPoints(const int index[3], int level, double cell[8][3]) const;

  /// Returns unique ids for cells' points associated with value at 'index'
  void GetPointIds(const int index[3], int pointIndexes[4]) const;
  void GetPointIds(const int index[3], int level, int pointIndexes[8]) const;

  /// Returns unique id for a cell at index
  int GetCellId(const int index[3]) const;
  int GetCellId(const int index[3], int level) const;

  /// converts from a (x,y,z) coordinate on the cube to an (s, t, face) index
  /// index is between (0,0,0) - (NLon-1, NLon-1, NLon-1). Returns the
  /// index in the vector that has the minimum face.
  size_t CubeToIndex(double side, const std::vector<double>& cubeCoordinates, double v[3],
    std::vector<std::vector<int> >& index) const;

  /// converts from (s, t, face) to cartesian coordinates on the cube
  /// the index is double[3] because we can get indexes in between integer positions.
  void IndexToCube(const std::vector<double>& cubeCoordinates, double index[3], double v[3]) const;
  void CubeToSpherical(double v[3], double* lonRad, double* latRad) const;

  /// converts 'index' to an index where the face is minimum. A conversion
  /// is made only for indexes that are shared between 2 or 3 faces.
  void MinFaceIndex(int side, int index[3]) const;

private:
  int MpiRank;          /// MPI rank
  int ChunkCapacity;    /// maximum number of (vertical) columns in a chunk
  int NCells2d;         /// total number of 2D cells on a MPI processor
  int NLon, NLat, NLev; /// number of unique longitude, latitude or level values.
                        /// for SE, NLon == NLat == number of values on the side
                        /// of the cube (number of elements + 1)
  double* Lev;          /// level values
  ///////////////////////////////////////////////////////////////////////////
  /// used for the SE dynamic core only
  double Radius; /// radius around a cube with side ne * np.
  int Ne;        /// the number of spectral  elements ne along the
                 /// edge of each cube face
  int Np;        /// each element is further subdivided by the Np x Np
                 /// Gauss-Lobatto-Legendre (GLL) quadrature
                 /// points where GLL stands for
  /// used for FV dynamic core only
  double LonStep; /// longitude step in degrees
  double LatStep; /// latitude step in degrees
  ///////////////////////////////////////////////////////////////////////////
  class Internals;
  Internals* Impl;

  /// 2d grid
  vtkSmartPointer<vtkUnstructuredGrid> Grid2d; // the grid
  int RankArrayIndex2d;                        // index of the rank array
  int CoordArrayIndex2d;                       // index of the coordinates array (lon x lat)
  int PSArrayIndex;                            /// index of the PS array

  /// 3d grid
  vtkSmartPointer<vtkUnstructuredGrid> Grid3d; /// the grid
  int RankArrayIndex3d;                        /// index of the rank array
  int CoordArrayIndex3d;                       /// index of the coordinates array (lon x lat x lev)
  int TArrayIndex;                             /// index of the attribute array for T
  int UArrayIndex;                             /// index of the attribute array for U
  int VArrayIndex;                             /// index of the attribute array for V
};
} // namespace CamAdaptor
#endif
