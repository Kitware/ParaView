/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTRenderManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkIceTRenderManager - The Image Composition Engine for Tiles (ICE-T) component for VTK.
//
// .SECTION Description
// A ParallelRenderManager object that uses the ICE-T library for compositing.
// As such, it offers the capability of rendering to tiled displays.  By
// default, it renders to a single tile located on the processor with
// rank 0.  That is, it mimics the operation of other CompositeManagers such
// as vtkTreeComposite.
//
// .SECTION Note
// In order for the the vtkIceTRenderManager class to work correctly, it
// can only be used with the vtkIceTRenderer instance of the vtkRenderer
// class (which requires OpenGL).  If any other renderer is used, warnings
// are emitted and no compositing is performed.  Creating the vtkRenderer
// with the MakeRenderer method (which you should do with any
// vtkParallelRenderManager) will ensure that the correct renderer is used.
//
// .SECTION Note
// Due to current limitations of the ICE-T API, only an instance of
// vtkMPIController will be accepted for a vtkMultiProcessController.
// This restriction may or may not be lifted in the future based on demand.
//
// .SECTION Bugs
// Expect bizarre behavior if using more than one renderer in the attached
// render window.
//
// .SECTION See Also
// vtkIceTRenderer

#ifndef __vtkIceTRenderManager_h
#define __vtkIceTRenderManager_h

#include "vtkParallelRenderManager.h"

class vtkPKdTree;
class vtkIntArray;

#include <GL/ice-t.h> // Needed for IceTContext

class VTK_EXPORT vtkIceTRenderManager : public vtkParallelRenderManager
{
public:
  static vtkIceTRenderManager *New();
  vtkTypeRevisionMacro(vtkIceTRenderManager, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual vtkRenderer *MakeRenderer();

  virtual void SetTileDimensions(int x, int y) 
    {this->SetNumTilesX(x); this->SetNumTilesY(y);}

  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // In addition to the superclass actions, this method
  // sets up the window for 2D Actors.
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Methods to set the characteristics of the tiled display.  Currently,
  // only an even grid is supported.  Numbering of tiles is 0 based.  Tiles
  // in the X direction (horizontal) are numbered from left to right.  Tiles
  // in the Y direction (vertical) are numbered from top to bottom.
  vtkGetMacro(NumTilesX, int);
  vtkGetMacro(NumTilesY, int);
  virtual void SetNumTilesX(int tilesX);
  virtual void SetNumTilesY(int tilesY);

  virtual int GetTileRank(int x, int y);
  virtual void SetTileRank(int x, int y, int rank);

  virtual double GetRenderTime();
  virtual double GetImageProcessingTime();
  virtual double GetBufferReadTime();
  virtual double GetBufferWriteTime();
  virtual double GetCompositeTime();

//BTX
  enum StrategyType {
    DEFAULT, REDUCE, VTREE, SPLIT, SERIAL, DIRECT
  };
//ETX

  // Description:
  // Methods to set the strategy.  The REDUCE strategy, which is also the
  // default, is a good all-around strategy.
  vtkGetMacro(Strategy, StrategyType);
  virtual void SetStrategy(StrategyType strategy);
  virtual void SetStrategy(const char *strategy);

//BTX
  enum ComposeOperationType {
    CLOSEST, OVER
  };
//ETX

  // Description:
  // Get/Set to operation to use when composing pixels together.  Note that
  // not all operations are commutative.  That is, for some operations, the
  // order of composition matters.
  vtkGetMacro(ComposeOperation, ComposeOperationType);
  virtual void SetComposeOperation(ComposeOperationType operation);
  // Description:
  // Sets the compose operation to pick the pixel color that is closest to
  // the camera (determined by the z-buffer).  This operation is
  // commutative.  This is the default operation.
  void SetComposeOperationToClosest() {
    this->SetComposeOperation(CLOSEST);
  }
  // Description:
  // Sets the compose operation to blend colors using the Porter and Duff
  // OVER operator.  This operation is not commutative (order of
  // composition matters).
  void SetComposeOperationToOver() {
    this->SetComposeOperation(OVER);
  }

  // Description:
  // Get/Set a parallel Kd-tree structure that will determine the order of
  // image composition.  If set to NULL (the default), no ordering will be
  // imposed.  Generally speaking, if the ComposeOperation is set to
  // CLOSEST, then giving an ordering is unnecessary.  If the
  // ComposeOperation is set to OVER, then an ordering is necessary.
  //
  // The given Kd-tree should have processes assigned to regions (the
  // default if created with the vtkDistributeDataFilter) and should have
  // the same controller as the one assigned to this object.  Furthermore,
  // the data held by each process should be strictly contained within the
  // Kd-tree regions it is assigned to (i.e. turn clipping on).
  vtkGetObjectMacro(SortingKdTree, vtkPKdTree);
  virtual void SetSortingKdTree(vtkPKdTree *tree);

  // Description:
  // Get/Set the data replication group.  The group comprises a list of
  // process IDs that contian the exact same data (geometry).  Replicating
  // data can reduce image composition time.  The local process ID should
  // be in the group and all processes within the group should have set the
  // exact same list in the same order.  This consistency is not checked,
  // but bad things can happen if it is not maintained.  By default, the
  // data replication group is set to a group containing only the local
  // process and is reset every time the controller is set.
  vtkGetObjectMacro(DataReplicationGroup, vtkIntArray);
  virtual void SetDataReplicationGroup(vtkIntArray *group);

  // Description:
  // An alternate way of setting the data replication group.  All processes
  // with the same color are assumed to be part of a data replication group
  // (that is, they all have the same geometry).  This method will not
  // return until it is called in all methods of the communicator.
  virtual void SetDataReplicationGroupColor(int color);

//BTX
  enum {
    ICET_INFO_TAG=234551,
    NUM_TILES_X_TAG=234552,
    NUM_TILES_Y_TAG=234553,
    TILE_RANKS_TAG=234554
  };
//ETX

protected:
  vtkIceTRenderManager();
  virtual ~vtkIceTRenderManager();

  virtual void UpdateIceTContext();

  virtual void ChangeTileDims(int tilesX, int tilesY);

  virtual void StartRender();
  virtual void SatelliteStartRender();

  virtual void SendWindowInformation();
  virtual void ReceiveWindowInformation();

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  IceTContext Context;
  int ContextDirty;
  vtkTimeStamp ContextUpdateTime;

  int NumTilesX;
  int NumTilesY;
  int **TileRanks;
  int TilesDirty;
  int CleanScreenWidth;
  int CleanScreenHeight;

  StrategyType Strategy;
  int StrategyDirty;

  ComposeOperationType ComposeOperation;
  int ComposeOperationDirty;

  vtkPKdTree *SortingKdTree;

  vtkIntArray *DataReplicationGroup;

  // Description:
  // Used to keep track of when the ImageReductionFactor changes, which
  // means the tiles have gotten dirty.
  int LastKnownImageReductionFactor;

  int FullImageSharesData;
  int ReducedImageSharesData;

  virtual void ReadReducedImage();

private:
  vtkIceTRenderManager(const vtkIceTRenderManager&); // Not implemented
  void operator=(const vtkIceTRenderManager&); // Not implemented
};


#endif //__vtkIceTRenderManager_h
