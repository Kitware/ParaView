/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTRenderer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTRenderer
// .SECTION Description
// The Renderer that needs to be used in conjunction with
// vtkIceTRenderManager object.
// .SECTION see also
// vtkIceTRenderManager

#ifndef __vtkIceTRenderer_h
#define __vtkIceTRenderer_h

#include "vtkOpenGLRenderer.h"

#include "vtkIceTRenderManager.h"       // For enumeration types.

class vtkIceTContext;

class VTK_EXPORT vtkIceTRenderer : public vtkOpenGLRenderer
{
public:
  static vtkIceTRenderer *New();
  vtkTypeRevisionMacro(vtkIceTRenderer, vtkOpenGLRenderer);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Override the regular device render.
  virtual void DeviceRender();

  // Description:
  // Basically a callback to be used internally.
  void RenderWithoutCamera();

  // Description:
  // Renders next frame with composition.  If not continually called, this
  // object behaves just like vtkOpenGLRenderer.  This is inteneded to be
  // called by vtkIceTRenderManager.
  vtkSetMacro(ComposeNextFrame, int);

  // Description:
  // Ensures that the background has an ambient color of 0 when color blend
  // compositing is on.
  virtual void Clear();

  // Description:
  // Reset ComposeNextFrame between rendering each eye for stereo viewing
  void StereoMidpoint();

  double GetRenderTime();
  double GetImageProcessingTime();
  double GetBufferReadTime();
  double GetBufferWriteTime();
  double GetCompositeTime();

  // Description:
  // Methods to set the strategy.  The REDUCE strategy, which is also the
  // default, is a good all-around strategy.
  vtkGetMacro(Strategy, int);
  vtkSetMacro(Strategy, int);
  void SetStrategyToDefault() {
    this->SetStrategy(vtkIceTRenderManager::DEFAULT);
  }
  void SetStrategyToReduce() {
    this->SetStrategy(vtkIceTRenderManager::REDUCE);
  }
  void SetStrategyToVtree() {
    this->SetStrategy(vtkIceTRenderManager::VTREE);
  }
  void SetStrategyToSplit() {
    this->SetStrategy(vtkIceTRenderManager::SPLIT);
  }
  void SetStrategyToSerial() {
    this->SetStrategy(vtkIceTRenderManager::SERIAL);
  }
  void SetStrategyToDirect() {
    this->SetStrategy(vtkIceTRenderManager::DIRECT);
  }

  // Description:
  // Get/Set to operation to use when composing pixels together.  Note that
  // not all operations are commutative.  That is, for some operations, the
  // order of composition matters.
  vtkGetMacro(ComposeOperation, int);
  vtkSetMacro(ComposeOperation, int);
  // Description:
  // Sets the compose operation to pick the pixel color that is closest to
  // the camera (determined by the z-buffer).  This operation is
  // commutative.  This is the default operation.
  void SetComposeOperationToClosest() {
    this->SetComposeOperation(vtkIceTRenderManager::ComposeOperationClosest);
  }
  // Description:
  // Sets the compose operation to blend colors using the Porter and Duff
  // OVER operator.  This operation is not commutative (order of
  // composition matters).
  void SetComposeOperationToOver() {
    this->SetComposeOperation(vtkIceTRenderManager::ComposeOperationOver);
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

  // Description:
  // Get the IceT context used by this renderer.
  vtkGetObjectMacro(Context, vtkIceTContext);

  // Description:
  // Use the given controller for IceT compositing.
  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // Returns the full size and origin of the viewport since IceT handles
  // the tile part itself.
  virtual void GetTiledSizeAndOrigin(int *width, int *height,
                                     int *lowerLeftX, int *lowerLeftY);

  // Description:
  // DO NOT USE.  Set by vtkIceTRenderManager.
  vtkSetVector4Macro(PhysicalViewport, int);
  vtkGetVector4Macro(PhysicalViewport, int);

  // Description:
  // Indicates if the depth buffer needs to be composited.
  // This is needed only for picking. Don't enable this otherwise, since it
  // causes considerable slow down.
  vtkSetMacro(CollectDepthBuffer, int);
  vtkGetMacro(CollectDepthBuffer, int);


protected:
  vtkIceTRenderer();
  virtual ~vtkIceTRenderer();

  virtual int UpdateCamera();
  virtual int UpdateGeometry();

  int ComposeNextFrame;
  int InIceTRender;

  int CollectDepthBuffer;
  int Strategy;
  int ComposeOperation;

  vtkIceTContext *Context;

  vtkPKdTree *SortingKdTree;

  vtkIntArray *DataReplicationGroup;

  int PhysicalViewport[4];

  // Used when rendering translucent geometry.
  bool *PropVisibility;

  // Description:
  // Ask all props to update and draw any translucent
  // geometry. This includes both vtkActors and vtkVolumes
  // Return the number of rendered props.
  // It is called once with alpha blending technique. It is called multiple
  // times with depth peeling technique.
  virtual int UpdateTranslucentPolygonalGeometry();

  vtkIceTRenderer(const vtkIceTRenderer&); // Not implemented
  void operator=(const vtkIceTRenderer&); // Not implemented
};

#endif //__vtkIceTRenderer_h
