/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHData - CTH AMR
// .SECTION Description
// vtkCTHData is a collection of image datas.  All have the same dimensions.
// Each block has a different origin and spacing. 

#ifndef __vtkCTHData_h
#define __vtkCTHData_h

#include "vtkDataSet.h"

#define VTK_CTH_DATA 10

class vtkDataArray;
class vtkFloatArray;
class vtkLine;
class vtkPixel;
class vtkVertex;
class vtkVoxel;
class vtkImageData;

class VTK_EXPORT vtkCTHData : public vtkDataSet
{
public:
  static vtkCTHData *New();

  vtkTypeRevisionMacro(vtkCTHData,vtkDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Access to blocks as image data.
  void GetBlock(int blockId, vtkImageData* block);

  // Description:
  // Copy the geometric and topological structure of an input image data
  // object.
  void CopyStructure(vtkDataSet *ds);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_CTH_DATA;};
  
  // Description:
  // Standard vtkDataSet API methods. See vtkDataSet for more information.
  vtkIdType GetNumberOfCells();
  vtkIdType GetNumberOfPoints();
  vtkIdType GetNumberOfCellsPerBlock();
  vtkIdType GetNumberOfPointsPerBlock();
  float *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType id, float x[3]);
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  void GetCellBounds(vtkIdType cellId, float bounds[6]);
  vtkIdType FindPoint(float x, float y, float z) { return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindPoint(float x[3]);
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkIdType cellId, float tol2, 
                     int& subId, float pcoords[3], float *weights);
  vtkIdType FindCell(float x[3], vtkCell *cell, vtkGenericCell *gencell,
                     vtkIdType cellId, float tol2, int& subId, 
                     float pcoords[3], float *weights);
  vtkCell *FindAndGetCell(float x[3], vtkCell *cell, vtkIdType cellId, 
                          float tol2, int& subId, float pcoords[3],
                          float *weights);
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int i, int j, int k);

  // Description:
  // Set dimensions of structured points dataset.
  void SetDimensions(int dims[3]);

  // Description:
  // Get dimensions of this structured points dataset.
  // Dimensions are computed from Extents during this call.
  int *GetDimensions();
  void GetDimensions(int dims[3]);

  // Description:
  // Different ways to set the extent of the data array.  The extent
  // should be set before the "Scalars" are set or allocated.
  // The Extent is stored  in the order (X, Y, Z).
  void SetExtent(int extent[6]);
  void SetExtent(int x1, int x2, int y1, int y2, int z1, int z2);
  vtkGetVector6Macro(Extent,int);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  int ComputeStructuredCoordinates(int blockId, float x[3], int ijk[3], float pcoords[3]);
  
  // Description:
  // Return the dimensionality of the data.
  int GetDataDimension();

  // Description:
  // Return the actual size of the data in kilobytes. This number
  // is valid only after the pipeline has updated. The memory size
  // returned is guaranteed to be greater than or equal to the
  // memory required to represent the data (e.g., extra space in
  // arrays, etc. are not included in the return value). THIS METHOD
  // IS THREAD SAFE.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Setting the number of blocks initializes the spacings and origins.
  void SetNumberOfBlocks(int num);
  int GetNumberOfBlocks();
  void SetBlockOrigin(int blockId, float* origin);
  void SetBlockOrigin(int blockId, float ox, float oy, float oz);
  float* GetBlockOrigin(int blockId);
  void SetBlockSpacing(int blockId, float* spacing);
  void SetBlockSpacing(int blockId, float sx, float sy, float sz);
  float* GetBlockSpacing(int blockId);

  // Description:
  // A way to add blocks if you do not know how many there will
  // eventually be.  Of course all blocks should be defined
  // before you start adding attribute arrays.
  // This call returns the next block id.  The new block
  // origin and spacing are initialized to 0.
  int InsertNextBlock();
  
  // Must only be called with vtkCTHData (or subclass) as input
  void CopyTypeSpecificInformation( vtkDataObject *image );

  // Description:
  // Shallow and Deep copy.
  void ShallowCopy(vtkDataObject *src);  
  void DeepCopy(vtkDataObject *src);

  void SetUpdateExtent(int piece, int numPieces,
                       int ghostLevel);
  void GetUpdateExtent(int &piece, int &numPieces,
                       int &ghostLevel);

  vtkSetMacro(NumberOfGhostLevels, int);
  vtkGetMacro(NumberOfGhostLevels, int);
  virtual void Initialize();

protected:
  vtkCTHData();
  ~vtkCTHData();

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  // The extent of what is currently in the structured grid.
  // Dimensions is just an array to return a value.
  // Its contents are out of data until GetDimensions is called.
  int Dimensions[3];
  int NumberOfGhostLevels;
  int DataDescription;

  vtkFloatArray* BlockOrigins;
  vtkFloatArray* BlockSpacings;

private:
  void InternalCTHDataCopy(vtkCTHData *src);
private:
  vtkCTHData(const vtkCTHData&);  // Not implemented.
  void operator=(const vtkCTHData&);  // Not implemented.
};


#endif



