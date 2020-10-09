/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotBlock.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpyPlotBlock
 * @brief   Represents a SpyPlot Block Grid
 *
 * vtkSpyPlotBlock is a regular hexahedral grid stored in a SpyPlot file.
 * The grid can be part of an Adaptive Mesh Refinement (AMR) dataset or
 * part
 * The class was extracted from vtkSpyPlotUniReader and expanded upon by
 * transisitioning functionality from vtkSpyPlotUniReader and adding it to
 * this class.  Note that this helper class is not derived from vtkObject
 * and can be allocated on the static
*/

#ifndef vtkSpyPlotBlock_h
#define vtkSpyPlotBlock_h

#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports
#include "vtkSystemIncludes.h"

class vtkCellData;
class vtkDoubleArray;
class vtkDataArray;
class vtkFloatArray;
class vtkSpyPlotIStream;
class vtkBoundingBox;
class vtkUnsignedCharArray;

class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotBlock
{
public:
  vtkSpyPlotBlock();
  ~vtkSpyPlotBlock();
  //@{
  /**

   */
  int GetLevel() const;
  void GetDimensions(int dims[3]) const;
  int GetDimension(int i) const;
  void GetBounds(double bounds[6]) const;
  void GetSpacing(double spacing[3]) const;
  void GetVectors(vtkDataArray* coordinates[3]) const;
  void GetVectors(vtkFloatArray* coordinates[3]) const;
  vtkFloatArray* GetVectors(int i) const;
  void GetExtents(int extents[6]) const;
  int IsAllocated() const;
  int IsFixed() const;
  int IsActive() const;
  int IsAMR() const;
  void MarkedAsFixed();
  void GetRealBounds(double realBounds[6]) const;
  int GetAMRInformation(const vtkBoundingBox& globalBounds, int* level, double spacing[3],
    double origin[3], int extents[6], int realExtents[6], int realDimensions[3]) const;
  // Read reads in the actual block's information
  int Read(int isAMR, int fileVersion, vtkSpyPlotIStream* stream);
  // Advances the stream to be after the block, information w/r
  // whether the block is allocated in the time step is returned
  // as an argument
  static int Scan(vtkSpyPlotIStream* stream, unsigned char* isAllocated, int fileVersion);
  // For performance reasons, lets do 16 at a time.
  static int Scan16(vtkSpyPlotIStream* stream, unsigned char* isAllocated, int fileVersion);
  //@}

  int SetGeometry(int dir, const unsigned char* encodedInfo, int infoSize);
  int GetTotalSize() const;
  int FixInformation(const vtkBoundingBox& globalBounds, int extents[6], int realExtents[6],
    int realDims[3], vtkDataArray* ca[3]);
  // Dummy functions so we can use vtk macros
  void SetDebug(unsigned char i);
  unsigned char GetDebug() const;

  void SetCoordinateSystem(const int& coordinateSystem);

  void ComputeDerivedVariables(vtkCellData* data, const int& numberOfMaterials,
    vtkDataArray** materialMasses, vtkDataArray** materialVolumeFractions,
    const int& downConvertVolumeFraction) const;

protected:
  // will return a negative volume if you request the volume of a cell
  // that is outside the dimensions of the block
  double GetCellVolume(int i, int j, int k) const;
  void ComputeMaterialDensity(vtkIdType position, vtkDataArray* materialMasses,
    vtkUnsignedCharArray* materialFraction, vtkDoubleArray* volumes,
    vtkDoubleArray* materialdensity, double* material_mass, double* material_volume) const;
  void ComputeMaterialDensity(vtkIdType position, vtkDataArray* materialMasses,
    vtkFloatArray* materialFraction, vtkDoubleArray* volumes, vtkDoubleArray* materialdensity,
    double* material_mass, double* material_volume) const;

  int Dimensions[3];
  struct BlockStatusType
  {
#ifndef __WRAP__
    unsigned Active : 1;
    unsigned Allocated : 1;
    unsigned AMR : 1;
    unsigned Fixed : 1;
    unsigned Debug : 1;
#endif
  };
  BlockStatusType Status;
  int Level;
  vtkFloatArray* XYZArrays[3];

  // for restoring structured once external ghost cells are removed
  int SavedExtents[6];
  int SavedRealExtents[6];
  int SavedRealDims[3];

private:
  enum CoordinateSystem
  {
    Cylinder1D = 11,
    Sphere1D = 12,
    Cartesian2D = 20,
    Cylinder2D = 21,
    Cartesian3D = 30
  };
  CoordinateSystem CoordSystem;
};

inline int vtkSpyPlotBlock::GetLevel() const
{
  return this->Level;
}

inline void vtkSpyPlotBlock::GetDimensions(int dims[3]) const
{
  dims[0] = this->Dimensions[0];
  dims[1] = this->Dimensions[1];
  dims[2] = this->Dimensions[2];
}

inline int vtkSpyPlotBlock::IsActive() const
{
  return this->Status.Active;
}

inline int vtkSpyPlotBlock::IsAllocated() const
{
  return this->Status.Allocated;
}

inline int vtkSpyPlotBlock::IsAMR() const
{
  return this->Status.AMR;
}

inline int vtkSpyPlotBlock::IsFixed() const
{
  return this->Status.Fixed;
}

inline void vtkSpyPlotBlock::MarkedAsFixed()
{
  this->Status.Fixed = 1;
}

inline int vtkSpyPlotBlock::GetDimension(int i) const
{
  return this->Dimensions[i];
}

inline void vtkSpyPlotBlock::GetVectors(vtkFloatArray* fa[3]) const
{
  fa[0] = this->XYZArrays[0];
  fa[1] = this->XYZArrays[1];
  fa[2] = this->XYZArrays[2];
}

inline vtkFloatArray* vtkSpyPlotBlock::GetVectors(int i) const
{
  return this->XYZArrays[i];
}

inline void vtkSpyPlotBlock::GetExtents(int extents[6]) const
{
  extents[0] = extents[2] = extents[4] = 0;
  extents[1] = (this->Dimensions[0] == 1) ? 0 : this->Dimensions[0];
  extents[3] = (this->Dimensions[1] == 1) ? 0 : this->Dimensions[1];
  extents[5] = (this->Dimensions[2] == 1) ? 0 : this->Dimensions[2];
}

inline int vtkSpyPlotBlock::GetTotalSize() const
{
  return (this->Dimensions[0] * this->Dimensions[1] * this->Dimensions[2]);
}

#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotBlock.h
