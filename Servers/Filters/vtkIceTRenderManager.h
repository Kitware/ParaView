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
// default, it renders to a single tile located on the processor with rank 0.
// That is, it mimics the operation of other vtkParallelRenderManagers such as
// vtkCompositeRenderManager.
//
// .SECTION Note
// In order for vtkIceTRenderManager class to composite images, it needs to be
// used in conjunction with a vtkIceTRenderer.  You can have multiple
// vtkIceTRenderers and they will correctly composite so long as they do not
// overlap.  If there are not vtkIceTRenderers attached to the render window, a
// warning is issued.  Creating a vtkRenderer with the MakeRenderer method
// (which you should do with any vtkParallelRenderManager) will ensure that the
// correct renderer is used.  You can also mix composited and non-composited
// viewports by adding regular vtkRenderers for the non-composited parts.  The
// non IceT renderers can overlap each other or the IceT renderers.  However,
// the non-IceT renderers must be on a higher level than all the IceT renderers.
//
// .SECTION Note
// Due to current limitations of the ICE-T API, only an instance of
// vtkMPIController will be accepted for a vtkMultiProcessController.
// This restriction may or may not be lifted in the future based on demand.
//
// .SECTION See Also
// vtkIceTRenderer

#ifndef __vtkIceTRenderManager_h
#define __vtkIceTRenderManager_h

#include "vtkParallelRenderManager.h"

#include "vtkIceTConstants.h"   // For constant definitions

class vtkIceTRenderer;
class vtkIntArray;
class vtkPerspectiveTransform;
class vtkPKdTree;
class vtkFloatArray;

class VTK_EXPORT vtkIceTRenderManager : public vtkParallelRenderManager
{
public:
  static vtkIceTRenderManager *New();
  vtkTypeMacro(vtkIceTRenderManager, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual vtkRenderer *MakeRenderer();

  virtual void SetController(vtkMultiProcessController *controller);

  virtual void SetRenderWindow(vtkRenderWindow *renwin);

  // Description:
  // Methods to set the characteristics of the tiled display.  Currently,
  // only an even grid is supported.  Numbering of tiles is 0 based.  Tiles
  // in the X direction (horizontal) are numbered from left to right.  Tiles
  // in the Y direction (vertical) are numbered from top to bottom.
  virtual void SetTileDimensions(int tilesX, int tilesY);
  void SetTileDimensions(int dims[2])
    { this->SetTileDimensions(dims[0], dims[1]); }
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Methods to set/get which processes is desplaying which tiles.
  // Currently, only an even grid is supported.  Numbering of tiles is 0
  // based.  Tiles in the X direction (horizontal) are numbered from left
  // to right.  Tiles in the Y direction (vertical) are numbered from top
  // to bottom.
  virtual int GetTileRank(int x, int y);
  virtual void SetTileRank(int x, int y, int rank);

  // Description:
  // Methods to set/get the mullions (space between the tiles).  The mullions
  // are measured in pixels.  Negative mullions correspond to overlap.
  virtual void SetTileMullions(int mullX, int mullY);
  void SetTileMullions(int mull[2])
    { this->SetTileMullions(mull[0], mull[1]); }
  vtkGetVector2Macro(TileMullions, int);

  // Description:
  // To use tile displays, it is essential to set this property to 1.
  void SetEnableTiles(int x);
  vtkGetMacro(EnableTiles, int);

  virtual double GetRenderTime();
  virtual double GetImageProcessingTime();
  virtual double GetBufferReadTime();
  virtual double GetBufferWriteTime();
  virtual double GetCompositeTime();

//BTX
  enum StrategyType {
    DEFAULT = vtkIceTConstants::DEFAULT,
    REDUCE  = vtkIceTConstants::REDUCE,
    VTREE   = vtkIceTConstants::VTREE,
    SPLIT   = vtkIceTConstants::SPLIT,
    SERIAL  = vtkIceTConstants::SERIAL,
    DIRECT  = vtkIceTConstants::DIRECT
  };
//ETX

  // Description:
  // Methods to set the strategy for all IceT renderers.  The REDUCE strategy,
  // which is also the default, is a good all-around strategy.
  virtual void SetStrategy(int strategy);
  virtual void SetStrategy(const char *strategy);
  void SetStrategyToDefault() { this->SetStrategy(DEFAULT); }
  void SetStrategyToReduce() { this->SetStrategy(REDUCE); }
  void SetStrategyToVtree() { this->SetStrategy(VTREE); }
  void SetStrategyToSplit() { this->SetStrategy(SPLIT); }
  void SetStrategyToSerial() { this->SetStrategy(SERIAL); }
  void SetStrategyToDirect() { this->SetStrategy(DIRECT); }

//BTX
  enum ComposeOperationType {
    ComposeOperationClosest = vtkIceTConstants::ComposeOperationClosest,
    ComposeOperationOver    = vtkIceTConstants::ComposeOperationOver
  };
//ETX

  // Description:
  // Set to operation to use when composing pixels together for all IceT
  // renderers.  Note that not all operations are commutative.  That is, for
  // some operations, the order of composition matters.
  virtual void SetComposeOperation(int operation);
  // Description:
  // Sets the compose operation to pick the pixel color that is closest to
  // the camera (determined by the z-buffer).  This operation is
  // commutative.  This is the default operation.
  void SetComposeOperationToClosest() {
    this->SetComposeOperation(ComposeOperationClosest);
  }
  // Description:
  // Sets the compose operation to blend colors using the Porter and Duff
  // OVER operator.  This operation is not commutative (order of
  // composition matters).
  void SetComposeOperationToOver() {
    this->SetComposeOperation(ComposeOperationOver);
  }

  // Description:
  // Set a parallel Kd-tree structure that will determine the order of image
  // composition for all IceT renderers.  If there is more than one
  // vtkIceTRenderer, each renderer should have its own sorting tree set
  // directly.  If set to NULL (the default), no ordering will be imposed.
  // Generally speaking, if the ComposeOperation is set to CLOSEST, then giving
  // an ordering is unnecessary.  If the ComposeOperation is set to OVER, then
  // an ordering is necessary.
  //
  // The given Kd-tree should have processes assigned to regions (the
  // default if created with the vtkDistributeDataFilter) and should have
  // the same controller as the one assigned to this object.  Furthermore,
  // the data held by each process should be strictly contained within the
  // Kd-tree regions it is assigned to (i.e. turn clipping on).
  virtual void SetSortingKdTree(vtkPKdTree *tree);

  // Description:
  // Set the data replication group for all IceT renderers.  If there is more
  // than one vtkIceTRenderer, each renderer should probably have its own data
  // replication group set directly.  The group comprises a list of process IDs
  // that contian the exact same data (geometry).  Replicating data can reduce
  // image composition time.  The local process ID should be in the group and
  // all processes within the group should have set the exact same list in the
  // same order.  This consistency is not checked, but bad things can happen if
  // it is not maintained.  By default, the data replication group is set to a
  // group containing only the local process and is reset every time the
  // controller is set.
  virtual void SetDataReplicationGroup(vtkIntArray *group);

  // Description:
  // An alternate way of setting the data replication group.  All processes
  // with the same color are assumed to be part of a data replication group
  // (that is, they all have the same geometry).  This method will not
  // return until it is called in all methods of the communicator.
  virtual void SetDataReplicationGroupColor(int color);

  // Description:
  // DO NOT USE.  FOR INTERNAL USE ONLY.  CONSULT A PHYSICIAN BEFORE USING.
  virtual void RecordIceTImage(vtkIceTRenderer *);

  // Description:
  // DO NOT USE.  FOR INTERNAL USE ONLY.  DO NOT EXCEED RECOMMENDED DOSAGE.
  virtual void ForceImageWriteback();

  // Description:
  // Returns the Z buffer value at the given location. Cannot be called on the
  // satellites, valid only on the root node.
  float GetZBufferValue(int x, int y);
protected:
  vtkIceTRenderManager();
  virtual ~vtkIceTRenderManager();

  virtual void UpdateIceTContext();

  virtual void CollectWindowInformation(vtkMultiProcessStream&);
  virtual bool ProcessWindowInformation(vtkMultiProcessStream&);

  virtual void CollectRendererInformation(vtkRenderer *, vtkMultiProcessStream&);
  virtual bool ProcessRendererInformation(vtkRenderer *, vtkMultiProcessStream&);

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  int ContextDirty;
  vtkTimeStamp ContextUpdateTime;

  int TileDimensions[2];
  int **TileRanks;
  int TileMullions[2];
  int TilesDirty;
  int CleanScreenWidth;
  int CleanScreenHeight;
  int EnableTiles;

  // Description:
  // Used to keep track of when the ImageReductionFactor changes, which
  // means the tiles have gotten dirty.
  double LastKnownImageReductionFactor;

  vtkCommand *RecordIceTImageCallback;
  vtkCommand *FixRenderWindowCallback;

  // Description:
  // Holds a transform that shifts a camera to the displayed viewport.
  vtkPerspectiveTransform *TileViewportTransform;
  vtkGetObjectMacro(TileViewportTransform, vtkPerspectiveTransform);
  virtual void SetTileViewportTransform(vtkPerspectiveTransform *arg);

  virtual void ComputeTileViewportTransform();

  virtual int ImageReduceRenderer(vtkRenderer *);

  // Description:
  // Keep around the last viewports so that we can rework the tiles if they
  // change.
  vtkDoubleArray *LastViewports;

  int PhysicalViewport[4];
  vtkFloatArray *ReducedZBuffer;

  // Description:
  // Convenience functions for determining IceT's logical viewports for
  // physical tiles.
  void GetGlobalViewport(int viewport[4]);
  void GetTileViewport(int x, int y, int viewport[4]);

private:
  vtkIceTRenderManager(const vtkIceTRenderManager&); // Not implemented
  void operator=(const vtkIceTRenderManager&); // Not implemented
};


#endif //__vtkIceTRenderManager_h
