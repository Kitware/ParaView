/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHData - CTH AMR
// .SECTION Description
// vtkCTHData is a collection of image datas.  
// Block arrays are concatenated. This structured is limited so the
// block spacings half at each aditional level.  Blocks are 
// placed relative to the global origin, level, and extent.
// Extent is defined by the levels spacing.
// The methods are currently implement so that all blocks must have 
// the same dimensions.  I should change that if we keep this class.

#ifndef __vtkCTHData_h
#define __vtkCTHData_h

#include "vtkDataSet.h"

#define VTK_CTH_DATA 1239

class vtkDataArray;
class vtkFloatArray;
class vtkIntArray;
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

  // Description:
  // Athough CTH has a contant number of cells per block, we would have to get
  // rid of this method if this data object were used for anything else.
  vtkIdType GetNumberOfCellsForBlock(int blockId);
  vtkIdType GetNumberOfPointsForBlock(int blockId);
  double *GetPoint(vtkIdType ptId);
  void GetPoint(vtkIdType id, double x[3]);
  vtkCell *GetCell(vtkIdType cellId);
  void GetCell(vtkIdType cellId, vtkGenericCell *cell);
  void GetCellBounds(vtkIdType cellId, double bounds[6]);
  vtkIdType FindPoint(double x, double y, double z) { return this->vtkDataSet::FindPoint(x, y, z);};
  vtkIdType FindPoint(double x[3]);
  vtkIdType FindCell(double x[3], vtkCell *cell, vtkIdType cellId, double tol2, 
                     int& subId, double pcoords[3], double *weights);
  vtkIdType FindCell(double x[3], vtkCell *cell, vtkGenericCell *gencell,
                     vtkIdType cellId, double tol2, int& subId, 
                     double pcoords[3], double *weights);
  vtkCell *FindAndGetCell(double x[3], vtkCell *cell, vtkIdType cellId, 
                          double tol2, int& subId, double pcoords[3],
                          double *weights);
  int GetCellType(vtkIdType cellId);
  void GetCellPoints(vtkIdType cellId, vtkIdList *ptIds);
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);
  void ComputeBounds();
  int GetMaxCellSize() {return 8;}; //voxel is the largest

  // Dimensions:
  // Specify blocks relative to this top level block.
  // For now this has to be set before the blocks are defined.
  vtkSetVector3Macro(TopLevelSpacing, double);
  vtkGetVector3Macro(TopLevelSpacing, double);
  vtkSetVector3Macro(TopLevelOrigin, double);
  vtkGetVector3Macro(TopLevelOrigin, double);

  // Description:
  // Convenience function computes the structured coordinates for a point x[3].
  // The voxel is specified by the array ijk[3], and the parametric coordinates
  // in the cell are specified with pcoords[3]. The function returns a 0 if the
  // point x is outside of the volume, and a 1 if inside the volume.
  int ComputeStructuredCoordinates(int blockId, double x[3], int ijk[3], double pcoords[3]);
  
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
  double* GetBlockSpacing(int blockId);

  // I am going to get rid of this origin, because
  // all blocks now share the same origin.
  double* GetBlockOrigin(int blockId);
  void GetBlockOrigin(int blockId, double origin[6]);

  // Description:
  // Level is an integer value that specifies the number of refinements
  // That occured to get to this block.  The spacing of this block
  // should be SpacingOfHighestLevel/2^level.  With the extent, 
  // it should no longer be necessary to explicitley store the spacing 
  // and origin of blocks.
  int GetBlockLevel(int blockId);

  // Description:
  // Set the blocks level and extent.
  void SetBlockCellExtent(int blockId, int level, int *extent);
  void SetBlockCellExtent(int blockId, int level, 
                          int eo, int e1, int e2, 
                          int e3, int e4, int e5);
  void SetBlockPointExtent(int blockId, int level, int *extent);
  void SetBlockPointExtent(int blockId, int level, 
                           int eo, int e1, int e2, 
                           int e3, int e4, int e5);
  void GetBlockPointDimensions(int blockId, int dims[3]);
  void GetBlockCellDimensions(int blockId, int dims[3]);
  void GetBlockPointExtent(int blockId, int ext[6]);
  int* GetBlockCellExtent(int blockId);
  void GetBlockCellExtent(int blockId, int ext[6]);

  // Description:
  // Convenience method that computes bounds from level and extent.
  void CellExtentToBounds(int level, int* ext, double bds[6]);

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

  // Description:
  // Extra methods for dealing with sets of blocks with different dimensions.
  vtkIdType GetBlockStartPointId(int blockId);
  vtkIdType GetBlockStartCellId(int blockId);
  int GetBlockIdFromPointId(vtkIdType ptId);
  int GetBlockIdFromCellId(vtkIdType ptId);

protected:
  vtkCTHData();
  ~vtkCTHData();

  void DeleteInternalArrays();
  void CreateInternalArrays();
  vtkIdType *BlockStartPointIds;
  vtkIdType *BlockStartCellIds;
  int *BlockNumberOfPoints;
  int *BlockNumberOfCells;
  float AverageNumberOfCellsPerBlock;
  float AverageNumberOfPointsPerBlock;

  // for the GetCell method
  vtkVertex *Vertex;
  vtkLine *Line;
  vtkPixel *Pixel;
  vtkVoxel *Voxel;

  int NumberOfGhostLevels;
  int DataDescription;

  // New method of specifing blocks.
  double TopLevelSpacing[3];
  double TopLevelOrigin[3];

  vtkIntArray* BlockCellExtents;
  vtkFloatArray* BlockOrigins;
  vtkFloatArray* BlockSpacings;
  vtkIntArray* BlockLevels;

  // Hidden methods
  void SetUpdateExtent(int, int ,int, int, int, int) {}
  void SetUpdateExtent(int*) {}
  void GetUpdateExtent(int &,int &,int &,int &,int &,int &) {}
  int* GetUpdateExtent() {return this->Superclass::GetUpdateExtent();}
  void GetUpdateExtent(int*) {}

  void GetExtent(int extent[6]);
private:
  void InternalCTHDataCopy(vtkCTHData *src);
private:
  vtkCTHData(const vtkCTHData&);  // Not implemented.
  void operator=(const vtkCTHData&);  // Not implemented.
};


#endif



