/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlashReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*****************************************************************************
*
* Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file was adapted from the VisIt Flash reader (avtFLASHFileFormat). For
* details, see https://visit.llnl.gov/.  The full copyright notice is contained
* in the file COPYRIGHT located at the root of the VisIt distribution or at
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/

// .NAME vtkFlashReader - Loads multi-resolution data blocks from a Flash file. 
//
// .SECTION Description
//  vtkFlashReader reads a Flash dataset and generates a vtkMultiBlockDataSet.
//  A Flash file may contain a hierarchy of multi-resolution vtkImageData
//  (2D / 3D) blocks, with the same dimension settings (in terms of the number
//  of grid points along each axis) across the sub-division tree, and/or a set
//  of particles (as a vtkPolyData block in the output), and the associated
//  scalar (cell) data attributes. vtkFlashReader exploits HDF5 libraries as 
//  the underlying data loading engine.
// 
// .SECTION See Also
//  vtkPolyData vtkImageData vtkMultiBlockDataSet

#ifndef __vtkFlashReader_h
#define __vtkFlashReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include <vtkstd/vector> // STL Header

class    vtkDataSet;
class    vtkPolyData;
class    vtkImageData;
class    vtkRectilinearGrid;
class    vtkMultiBlockDataSet;
class    vtkFlashReaderInternal;
class    vtkDataArraySelection;
class    vtkCallbackCommand;
class    vtkDataSetAttributes;

class VTK_EXPORT vtkFlashReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkFlashReader * New();
  vtkTypeMacro( vtkFlashReader, vtkMultiBlockDataSetAlgorithm );
  void PrintSelf( ostream & os, vtkIndent indent );

  // Description:
  // If true, the reader will merge scalar arrays named, for example, "X velocity"
  // "Y velocity" and "Z velocity" into a vector array named "velocity" with
  // scalar components X, Y and Z. It will also merge X and Y scalar arrays
  // (with no Z component) into a vector with scalar components X, Y and 0.
  // True by default.
  void SetMergeXYZComponents(int merge);
  vtkGetMacro(MergeXYZComponents,int);
  vtkBooleanMacro(MergeXYZComponents,int);

  
  // Description:
  // Set the Flash data file name.
  void           SetFileName( const char * fileName );
  
  // Description:
  // Set the output type of each block to vtkImageData (note that this type
  // is imposed on the constituent MAIN blocks, excluding the one constrcted
  // for the particles, if any).
  void           SetBlockOutputTypeToImageData()
                 { this->SetBlockOutputType( 0 ); }
                 
  // Description:
  // Set the output type of each block to vtkRectilinearGrid (note that this 
  // type is imposed on the constituent MAIN blocks, excluding the one built
  // for the particles, if any).
  void           SetBlockOutputTypeToRectilinearGrid()
                 { this->SetBlockOutputType( 1 ); }
  
  // Description:
  // Set/Get the output type of each block (vtkImageData or vtkRectilinearGrid),
  // with vtkImageData as the default. (note that this type is imposed on the 
  // constituent MAIN blocks, excluding the one built for the particles, if any).
  vtkSetClampMacro( BlockOutputType, int, 0, 1 );
  vtkGetMacro( BlockOutputType, int );

  // Description:
  // Do not load mor than this number of blocks.
  // Right now parent blocks are loaded, but they do not need to be.
  vtkSetMacro( MaximumNumberOfBlocks,int);
  
  // Description:
  // Description:
  // An interface to select blocks to load based on a camera.
  // Blocks containing the point will be refined as much as possible.
  // We could have multiple points.  Blocks near the point
  // are preferentially refined. (See also MaximumNumberOfBlocks).
  // Set position of first place to refine.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Set position of the second point to refine.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);  
  
  // Description:
  // Load the morton curve.
  vtkSetMacro( LoadMortonCurve, int );
  vtkGetMacro( LoadMortonCurve, int );
  vtkBooleanMacro( LoadMortonCurve, int );
  
  // Description:
  // Load the particles.
  vtkSetMacro( LoadParticles, int );
  vtkGetMacro( LoadParticles, int );
  vtkBooleanMacro( LoadParticles, int );
  
  // --------------------------------------------------------------------------
  // --------------------------- General Information --------------------------
  
  // Description:
  // Get the number of all (root, intermediate, and leaf) rectilinear blocks, 
  // excluding particles and the constructed Morton (z-order) curve.
  int            GetNumberOfBlocks();
  
  // Description:
  // Get the number of (sub-division) levels (including the root and the leaf).
  int            GetNumberOfLevels();
  
  // Description:
  // Get the version of the Flash file format.
  int            GetFileFormatVersion();
  
  // Description:
  // Get the number of particles.
  int            GetNumberOfParticles();
  
  // Description:
  // Get the number of leaf blocks.
  int            GetNumberOfLeafBlocks();
  
  // Description:
  // Get the number of dimensions (2 or 3).
  int            GetNumberOfDimensions();
  
  // Description:
  // Get the number of processors (used to generate the data).
  int            GetNumberOfProcessors();
  
  // Description:
  // Check whether the processors information is available (1) or not (0).
  int            HaveProcessorsInformation();
  
  // Description:
  // Get the dimensions of each block (throughout the hierarchy), i.e., the
  // number of grid points along each axis. 
  const int    * GetDimensionsPerBlock();
  
  // Description:
  // Get the dimensions of each block (throughout the hierarchy), i.e., the
  // number of grid points along each axis. 
  void           GetDimensionsPerBlock( int BlckDims[3] );
  
  // Description:
  // Get the number of childrens (4 for 2D and 8 for 3D) of each non-leaf block.
  int            GetNumberOfChildrenPerBlock();
  
  // Description:
  // Get the maximum number of neighbors (4 for 2D and 6 for 3D) of each block.
  int            GetNumberOfNeighborsPerBlock();
  
  // Description:
  // Get the bounding box of the whole hierarchy (covering all rectilinear
  // blocks).
  void           GetBounds( double dataBBox[6] );
  
  // --------------------------------------------------------------------------
  // ---------------------- Information About Each Block ----------------------
  // --- each block is accessed via 0-based Id  instead of 1-based Flash Id ---
  
  // Description:
  // Get the 1-based Flash Id of a block specified by 0-based blockIdx. A 
  // negative value is returned for invalid blockIdx.
  int            GetBlockFlashId( int blockIdx );
  
  // Description:
  // Get the 1-based level of a block specified by 0-based blockIdx. The level
  // increases as we navigate top-down from the root block. A negative value is
  // returned for invalid blockIdx.
  int            GetBlockLevel( int blockIdx );
  
  // Description:
  // Get the type (1 for a leaf, 2 for an intermediate, and 3 for the root) of 
  // a block specified by 0-based blockIdx. A negative value is returned for 
  // invalid blockIdx.
  int            GetBlockType( int blockIdx );
  
  // Description:
  // Get the 1-based Flash Id of the parent of a block specified by 0-based
  // blockIdx. -2 is returned for invalid blockIdx while -1 is returned for
  // the root block as the input.
  int            GetBlockParentId( int blockIdx );
  
  // Description:
  // Get 1-based Flash Ids of the children of a block specified by 0-based
  // blockIdx. NULL is returned for invalid blockIdx. A pointer to an array
  // of { -1, -1, -1, -1, -1, -1, -1, -1 } is returned for an input leaf.
  const int    * GetBlockChildrenIds( int blockIdx );
  
  // Description:
  // Get 1-based Flash Ids of the children of a block specified by 0-based
  // blockIdx. { -1, -1, -1, -1, -1, -1, -1, -1 } is returned for invalid
  // blockIdx or an input leaf.
  void           GetBlockChildrenIds( int blockIdx, int childIds[8] );
  
  // Description:
  // Get 1-based Flash Ids of the neighbors of a block specified by 0-based
  // blockIdx. NULL is returned for invalid blockIdx. A pointer to an array
  // of { -32, -32, -32, -32, -32, -32 } is returned for the root block as
  // the input.
  const int    * GetBlockNeighborIds( int blockIdx );
  
  // Description:
  // Get 1-based Flash Ids of the neighbors of a block specified by 0-based
  // blockIdx. { -32, -32, -32, -32, -32, -32 } is returned for invalid blockIdx
  // or the root block as the inpur.
  void           GetBlockNeighborIds( int blockIdx, int neighbrs[6] );
  
  // Description:
  // Get the 0-based processor Id of a block specified by 0-based blockIdx. -1 
  // is returned for invalid blockIdx.
  int            GetBlockProcessorId( int blockIdx );
  
  // Description:
  // Get the center of a block specified by 0-based blockIdx. NULL is returned
  // for invalid blockIdx.
  const double * GetBlockCenter( int blockIdx );
  
  // Description:
  // Get the center of a block specified by 0-based blockIdx. { -VTK_DOUBLE_MAX,
  // -VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX } is returned for invalid blockIdx.
  void           GetBlockCenter( int blockIdx, double blockMid[3] );
  
  // Description:
  // Get the bounding box of a block specified by 0-based blockIdx. For invalid
  // blockIdx, { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX,
  // VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX } is returned.
  void           GetBlockBounds( int blockIdx, double dataBBox[6] );
  
  // Description:
  // Get the 0-based Id (not the 1-based Flash Id) of the leafIndx-th (0-based)
  // leaf block. -1 is returned for invalid leafIndx.
  int            GetLeafBlockId( int leafIndx );
  
  // Description:
  // Get the 1-based Flash Id of the leafIndx-th (0-based) leaf block. -1 is 
  // returned for invalid leafIndx.
  int            GetLeafBlockIdFlash( int leafIndx );
  
  // Description:
  // Check whether a block specified by 0-based blockIdx is a leaf (1) or not (0).
  int            IsLeafBlock( int blockIdx );
  
  // Description:
  // Check whether a block specified by 0-based blockIdx is an intermediate (1) 
  // or not (0). An intermediate block is one that is neither the root nor a
  // leaf block.
  int            IsIntermediateBlock( int blockIdx );
  
  // Description:
  // Get the name of particles.
  const char   * GetParticleName();
  
  // Description:
  // Get the number of data attributes associated with particles.
  int            GetNumberOfParticleAttributes();
  
  // Description:
  // Get the name of the attrIndx-th (0-based) scalar data attribute associated
  // with particles. NULL is returned for invalid attrIndx.
  const char   * GetParticleAttributeName( int attrIndx );
  
  // Description:
  // Check whether a given name attrName is a data attribute associated with
  // particles (return a non-negative value as the attribute index)or not (-1).
  int            IsParticleAttribute( const char * attrName );    
  
  // Description:
  // This function fills an allocated vtkImageData with a block specified by
  // 0-based blockIdx and loads the associated cell data attributes from the
  // file. The returned value indicates failure (0) or success (1).
  int            GetBlock( int blockIdx, vtkImageData * imagData );     
  
  // Description:
  // This function fills an allocated vtkImageData with a block specified
  // by 0-based blockIdx and loads the associated cell data attributes from the
  // file. The returned value indicates failure (0) or success (1).
  int            GetBlock( int blockIdx, vtkRectilinearGrid * rectGrid );
  
  // Description:
  // This function loads particles (and the associated data attributes) from the 
  // file and fills an allocated vtkPolyDara with them. The returned value 
  // indicates failure (0) or success (1).
  int            GetParticles( vtkPolyData * polyData );
  
  // Description:
  // This function creates a morton (or usually called z-order) curve connecting 
  // all the leaf rectilinear blocks by their centers. This morton curve is then
  // stored in an allocated vtkPolyData. The returned value indicates failure (0)
  // or success (1).
  int            GetMortonCurve( vtkPolyData * polyData );
  






  // Description:
  // Get the number of (cell) data attributes associated with each rectilinear
  // block.
  int            GetNumberOfBlockAttributes();
  
  // Description:
  // Check whether a given name attrName is a (cell) data attribute associated
  // with each block (return a non-negative value as the attribute index)or not
  // (-1).
  int            IsBlockAttribute( const char * attrName );
  
  // Description:
  // Get the name of the attrIndx-th (0-based) scalar (cell) data attribute
  // associated with each block. NULL is returned for invalid attrIndx.
  const char   * GetBlockAttributeName( int attrIndx );
  
  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  // Description:
  // Cell array selection
  int GetNumberOfCellArrays() {return this->GetNumberOfBlockAttributes();}
  const char* GetCellArrayName(int idx) {return this->GetBlockAttributeName(idx);}
  int GetCellArrayStatus(const char *name);
  void SetCellArrayStatus(const char *name, int status);  

protected:
  vtkFlashReader();
  ~vtkFlashReader();
  
  // The file format stores a vector field as separated scalar component
  // fields. This method rebuilds the vector field from those scalar
  // component fields.
  void MergeVectors(vtkDataSetAttributes *da);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2,
                   vtkDataArray *a3);
  
  int MergeXYZComponents;



  // The array selections.
  vtkDataArraySelection *CellDataArraySelection;
  vtkCallbackCommand *SelectionObserver;

  // Description:
  // This does the updating of meta data of the dataset from the
  // first binary file registered in the map:
  // - number of time steps
  // - number of fields
  // - name of fields
  int UpdateMetaData(vtkInformation* request,
                     vtkInformationVector* outputVector);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);










  double Point1[3];
  double Point2[3];
  
  // Description:
  // This function creates a vtkImageData block (and loads the associated 
  // cell data attributes from the file) specified by 0-based blockIdx and 
  // inserted it to an allocated vtkMultiBlockDataSet multiBlk.
  void           GetBlock( int blockIdx, vtkMultiBlockDataSet * multiBlk );
  
  // Description:
  // This function, called by GetBlock( ... ), loads from the file a cell data
  // attribute (with name atribute) associated with a vtkRectilinerGrid block
  // specified by 0-based blockIdx and inserts it to an allocated vtkDataSet
  // pDataSet (which is either vtkImageData or vtkRectilinearGrid).
  void           GetBlockAttribute( const char * atribute, int blockIdx, 
                                    vtkDataSet * pDataSet );
                                  
  // Description:
  // This function loads particles (and the associated data attributes) from the 
  // file and insert them as a vtkPolyData (with blockIdx as the assigned 0-based
  // block index) to an allocated vtkMultiBlockDataSet multiBlk.
  void           GetParticles( int & blockIdx, vtkMultiBlockDataSet * multiBlk );
  
  // Description:
  // This function, called by GetParticles( ... ), loads a data attribute (with
  // name atribute) associated with the particles in the form of a vtkPolyData
  // polyData.
  void           GetParticlesAttribute( const char  * atribute, 
                                        vtkPolyData * polyData );
                                      
  // Description:
  // This function creates a morton (or usually called z-order) curve connecting 
  // all the leaf rectilinear blocks by their centers. This morton curve is 
  // inserted in the form of a vtkPolyData (with blockIdx as the assigned 0-based
  // block index) to an allocated vtkMultiBlockDataSet multiBlk.
  void           GetMortonCurve( int & blockIdx, vtkMultiBlockDataSet * multiBlk );
  
  // Description:
  // This function creates two line segments, as part of the morton curve, for
  // a leaf rectilinear block (specified by 0-based blockIdx) in relation with 
  // the two neighboring leaf rectilinear blocks. These two segments are stored
  // in an allocated vtkPolyData polyData.
  int            GetMortonSegment( int blockIdx, vtkPolyData * polyData );
  
  virtual int    FillOutputPortInformation( int port, vtkInformation * info );
  int            RequestData( vtkInformation *,
                              vtkInformationVector **, vtkInformationVector * );
  int            RequestInformation(vtkInformation *request,
                                    vtkInformationVector **inputVector,
                                    vtkInformationVector *outputVector);
  
  
  char *         FileName;
  static int     NumberOfInstances;
  vtkFlashReaderInternal * Internal;
  int            BlockOutputType;
  int            LoadMortonCurve;
  int            LoadParticles;
  int            MaximumNumberOfBlocks;
  
//BTX
  vtkstd::vector<int>    ToGlobalBlockMap;
  vtkstd::vector<double> BlockRank;
  // Every process to have the same blocks.  They may be empty but they have to be the same.
  // Keep track of whichprocess will actually load the block.
  vtkstd::vector<int>    BlockProcess;
//ETX
  virtual void GenerateBlockMap();
  void AddBlockToMap(int globalId);
  
  // Files are split up between processes as whole trees.
  // Keep track of the number of roots so we know how to 
  // assigne the trees.  This is set in the RequestInformation method.
  int NumberOfRoots;
  int MyProcessId;
                            
private:

  vtkFlashReader( const vtkFlashReader & );    // Not implemented.
  void operator = ( const vtkFlashReader & );  // Not implemented.
};

#endif // __vtkFlashReader_h
