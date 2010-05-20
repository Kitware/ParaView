/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnzoReader.h

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
* This file was adapted from the VisIt Enzo reader (avtEnzoFileFormat). For
* details, see https://visit.llnl.gov/.  The full copyright notice is contained
* in the file COPYRIGHT located at the root of the VisIt distribution or at
* http://www.llnl.gov/visit/copyright.html.
*
*****************************************************************************/

// .NAME vtkEnzoReader - Loads multi-resolution data blocks from an Enzo file. 
//
// .SECTION Description
//  vtkEnzoReader reads an Enzo dataset and generates a vtkMultiBlockDataSet.
//  An Enzo file may contain a hierarchy of multi-resolution vtkRectilinearGrid
//  (2D / 3D) blocks, with the same dimension settings (in terms of the number
//  of grid points along each axis) across the sub-division tree, and/or a set
//  of particles (as a vtkPolyData block in the output), and the associated
//  scalar (cell) data attributes. vtkEnzoReader exploits HDF5 libraries as 
//  the underlying data loading engine.
// 
// .SECTION See Also
//  vtkPolyData vtkImageData vtkRectilinearGrid vtkMultiBlockDataSet

#ifndef __vtkEnzoReader_h
#define __vtkEnzoReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"

#include <vtkstd/vector> // STL Header

class    vtkDataSet;
class    vtkPolyData;
class    vtkDataArray;
class    vtkImageData;
class    vtkDoubleArray;
class    vtkRectilinearGrid;
class    vtkMultiBlockDataSet;
class    vtkEnzoReaderInternal;

class VTK_EXPORT vtkEnzoReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkEnzoReader * New();
  vtkTypeMacro( vtkEnzoReader, vtkMultiBlockDataSetAlgorithm );
  void PrintSelf( ostream & os, vtkIndent indent );
  
  // Description:
  // Set the level, up to which the blocks are loaded.
  vtkSetMacro( MaxLevel,int);
  
  // Description:
  // Set/Get the output type of each rectilinear block (vtkImageData or 
  // vtkRectilinearGrid), with vtkImageData as the default. (note that this type
  // is imposed on the constituent MAIN blocks, excluding the one built for the
  // particles, if any).
  vtkSetClampMacro( BlockOutputType, int, 0, 1 );
  vtkGetMacro( BlockOutputType, int );
  
  // Description:
  // Set/Get whether the particles, if any, are loaded
  vtkSetMacro( LoadParticles, int );
  vtkGetMacro( LoadParticles, int );
  vtkBooleanMacro( LoadParticles, int );
  
  // Description:
  // Set the Enzo data file name (hierarchy or boundary).
  void           SetFileName( const char * fileName );
  
  // --------------------------------------------------------------------------
  // --------------------------- General Information --------------------------
  
  // Description:
  // Get the number of all rectilinear blocks, excluding particles.
  int            GetNumberOfBlocks();
  
  // Description:
  // Get the number of block levels (from the top level --- #0 to the lowest
  // leaf level).
  int            GetNumberOfLevels();
  
  // Description:
  // Get the number of leaf blocks.
  int            GetNumberOfLeafBlocks();
  
  // Description:
  // Get the number of dimensions (2 or 3).
  int            GetNumberOfDimensions();
  
  // Description:
  // Get the bounding box of the whole hierarchy (covering all rectilinear
  // blocks). The bounding range along the degenerate dimension (e.g., the
  // z-axis for 2D blocks) is invalid --- [ VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX ].
  void           GetBounds( double dataBBox[6] );
  
  // --------------------------------------------------------------------------
  // ---------------------- Information About Each Block ----------------------
  // ---- each block is accessed via 0-based Id instead of 1-based Enzo Id ----
  
  // Description:
  // Get the 1-based Enzo Id of a block specified by 0-based blockIdx. A negative
  // value is returned for invalid blockIdx.
  int            GetBlockEnzoId( int blockIdx );
  
  // Description:
  // Get the 0-based level of a block specified by 0-based blockIdx (there can
  // be multiple blocks at level #0). The level increases as we navigate top-
  // down through the hierarchy. A negative value is returned for invalid blockIdx.
  int            GetBlockLevel( int blockIdx );
  
  // Description:
  // Get the type (0 for level #0, 1 for an intermediate level, and 2 for a leaf)
  // of a block specified by 0-based blockIdx. A negative value is returned for 
  // invalid blockIdx.
  int            GetBlockType( int blockIdx );
  
  // Description:
  // Get the 0-based Id of the parent of a block specified by 0-based blockIdx. 
  // -2 is returned for invalid blockIdx while -1 is returned if the input refers 
  // to a level #0 block.
  int            GetBlockParentId( int blockIdx );
  
  // Description:
  // Get the number of child blocks of a block specified by 0-based blockIdx. -1
  // is returned for invalid blockIdx.
  int            GetBlockNumberOfChildren( int blockIdx );
  
  // Description:
  // Get the 0-based Id of the childIdx-th child of a block specified by 0-based
  // blockIdx. -1 is returned for invalid childIdx or blockIdx.
  int            GetBlockChild( int blockIdx, int childIdx );
  
  // Description:
  // Get the bounding box of a block specified by 0-based blockIdx. For invalid
  // blockIdx, { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX,
  // VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX } is returned.
  void           GetBlockBounds( int blockIdx, double dataBBox[6] );
  
  // Description:
  // Get the sub-division ratio, along each of the three axes (current block vs. 
  // the parent block in the number of cells alng each axis), of a block specified
  // by 0-based blockIdx. NULL is returned for invalid blockIdx.
  const double * GetBlockSubdivisionRatios( int blockIdx );
  
  // Description:
  // Get the 6-Id extents (min_x_Id, max_x_Id, min_y_Id, max_y_Id, min_z_Id,
  // max_z_Id) of a block (specified by 0-based blockIdx) in terms of its parent
  // block's node indexing. (-1, -1, -1, -1, -1, -1) is returned for invalid
  // blockIdx.
  void           GetBlockParentWiseNodeExtents( int blockIdx, int xtentIds[6] );
  
  // Description:
  // Get the 6-Id extents (min_x_Id, max_x_Id, min_y_Id, max_y_Id, min_z_Id,
  // max_z_Id) of a block (specified by 0-based blockIdx) in terms of the node
  // indexing across all the blocks at the same level. (-1, -1, -1, -1, -1, -1)
  // is returned for invalid blockIdx.
  void           GetBlockLevelBasedNodeExtents( int blockIdx, int xtentIds[6] );
  
  // Description:
  // Get the node dimensions (number of nodes along each of the three axes) of 
  // a block specified by 0-based blockIdx. NULL is returned for invalid blockIdx.
  const int *    GetBlockNodeDimensions( int blockIdx );
  
  // Description:
  // Get the cell dimensions (number of cells along each of the three axes) of 
  // a block specified by 0-based blockIdx. NULL is returned for invalid blockIdx.
  const int *    GetBlockCellDimensions( int blockIdx );
  
  // Description:
  // Get the block file name of a rectilinear block specified by 0-based blockIdx.
  // NULL is returned for invalid blockIdx.
  const char *   GetBlockFileName( int blockIdx );
  
  // Description:
  // Get the particle file name of the particles falling within the scope of a
  // block specified by 0-based blockIdx. NULL is returned for invalid blockIdx.
  const char *   GetParticleFileName( int blockIdx );
  
  // Description:
  // Get the number of particles falling within the scope of a block specified
  // by 0-based blockIdx. A negative value is returned for invalid blockIdx.
  int            GetNumberOfParticles( int blockIdx );
  
  // Description:
  // Get the number of (cell) data attributes associated with each rectilinear
  // block.
  int            GetNumberOfBlockAttributes();
  
  // Description:
  // Get the name of the attrIndx-th (0-based) scalar (cell) data attribute
  // associated with each block. NULL is returned for invalid attrIndx.
  const char   * GetBlockAttributeName( int attrIndx );
  
  // Description:
  // Check whether a given name attrName is a (cell) data attribute associated
  // with each block (return a non-negative value as the attribute index) or not
  // (-1).
  int            IsBlockAttribute( const char * attrName );
  
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
  // Get the number of data attributes associated with tracer particles.
  int            GetNumberOfTracerParticleAttributes();
  
  // Description:
  // Get the name of the attrIndx-th (0-based) scalar data attribute associated
  // with tracer particles. NULL is returned for invalid attrIndx.
  const char   * GetTracerParticleAttributeName( int attrIndx );
  
  // Description:
  // Check whether a given name attrName is a data attribute associated with
  // tracer particles (return a non-negative value as the attribute index) or 
  // not (-1).
  int            IsTracerParticleAttribute( const char * attrName );
  
  // Description:
  // Set the output type of each rectilinear block to vtkImageData (note that
  // this type is imposed on the constituent MAIN blocks, excluding the one 
  // constrcted for the particles, if any).
  void           SetBlockOutputTypeToImageData()
                 { this->SetBlockOutputType( 0 ); }
                 
  // Description:
  // Set the output type of each rectilinear block to vtkRectilinearGrid (note 
  // that this type is imposed on the constituent MAIN blocks, excluding the 
  // one built for the particles, if any).
  void           SetBlockOutputTypeToRectilinearGrid()
                 { this->SetBlockOutputType( 1 ); }
  
  // Description:
  // This function fills an allocated vtkImageData with a block specified by
  // 0-based blockIdx and loads the associated cell data attributes from the
  // file. The returned value indicates failure (0) or success (1).
  int            GetBlock( int blockIdx, vtkImageData * imagData );  
  
  // Description:
  // This function fills an allocated vtkRectilinearGrid with a block specified
  // by 0-based blockIdx and loads the associated cell data attributes from the
  // file. The returned value indicates failure (0) or success (1).
  int            GetBlock( int blockIdx, vtkRectilinearGrid * rectGrid );
  
  // Description:
  // This function loads particles (and the associated data attributes) from the 
  // file that fall within the scope of a rectilinear block specified by 0-based
  // blockIdx and fills an allocated vtkPolyDara with them. The returned value 
  // indicates failure (0) or success (1). Argument beTracer indicates either 
  // regular (0) or tracer (1) particles. Argument withAttr indicates whether 
  // attributes are loaded and attached (1) or not (0).
  int            GetParticles( int blockIdx, vtkPolyData * polyData, 
                               int beTracer = 0, int withAttr = 1 );
                               
  // Description:
  // This function loads from the file a cell data attribute (specified by 
  // name atribute, coupled with either the cells of a rectilinear block or
  // a set of particles falling within the scope of this rectilinear block)
  // associated with a block given by 0-based blockIdx. NULL is returned 
  // upon failure. Note that the array must NOT be deleted and instead it 
  // will be internally destroyed upon the next call to GetAttribute() / 
  // LoadAttribute(), either directly or indirectly. Thus it is suggested 
  // that the returned array be deep-copied for safety. Otherwise it can be
  // used for instantaneous access / check purposes only.
  vtkDataArray * GetAttribute( const char * atribute, int blockIdx );
  
protected:
  vtkEnzoReader();
  ~vtkEnzoReader();
  
  // Description:
  // This function creates a rectilinear block (and loads the associated 
  // cell data attributes from the file) specified by 0-based mapIndex
  // (referencing an Id-map for the actual 0-based blockId while the Id-map
  // is used for extracting blocks with the levels less than a given value)
  // and insert the resulting vtkImageData or vtkRectilinearGrid object to 
  // an allocated vtkMultiBlockDataSet multiBlk. If the 'LoadParticles' flag
  // is on, this function additionally creates a vtkPolyData to load the
  // particles (and the associated attributes) falling within the scope of 
  // this recilinear block and insert it to the vtkMultiBlockDataSet.
  void           GetBlock( int mapIndex, vtkMultiBlockDataSet * multiBlk );
  
  // Description:
  // This function, called by GetBlockAttribute() or GetParticleAttribute(), 
  // loads from the file a cell data attribute (specified by name atribute, 
  // assigned to either cells of a rectilinear block or individual vertices / 
  // particles of a vtkPolyData) associated with a block given by 0-based 
  // blockIdx. The returned value indicates failure (0) or success (1).
  int            LoadAttribute( const char * atribute, int blockIdx );
  
  // Description:
  // This function, called by GetBlock( ... ), loads from the file a cell data
  // attribute (with name atribute) associated with a rectiliner block specified
  // by 0-based blockIdx and inserts it to an allocated vtkDataSet pDataSet 
  // (which is either vtkImageData or vtkRectilinearGrid). The returned value
  // indicates failure (0) or success (1).
  int            GetBlockAttribute( const char * atribute, int blockIdx, 
                                    vtkDataSet * pDataSet );
                                    
  // Description:
  // This function, called by GetParticles( ... ), loads a data attribute (with
  // name atribute) associated with the particles falling within the scope of a
  // rectilinear block (specified by 0-based blockIdx) to a vtkPolyData.
  int            GetParticlesAttribute( const char  * atribute, int blockIdx,
                                        vtkPolyData * polyData );
                                    
  virtual int    FillOutputPortInformation( int port, vtkInformation * info );
  int            RequestData( vtkInformation *,
                              vtkInformationVector **, vtkInformationVector * );
                              
//BTX
  friend  class  vtkEnzoReaderInternal; // to call LoadAttribute()
//ETX

  vtkEnzoReaderInternal * Internal;
  int            BlockOutputType;
  int            LoadParticles;
  int            MaxLevel;
  char         * FileName;
  
//BTX
  vtkstd::vector<int> BlockMap;
//ETX
  virtual void GenerateBlockMap();
                            
private:

  vtkEnzoReader( const vtkEnzoReader & );     // Not implemented.
  void operator = ( const vtkEnzoReader & );  // Not implemented.
};

#endif // __vtkEnzoReader_h
