/*=========================================================================

  Program:   ParaView
  Module:    vtkCaveRenderManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCaveRenderManager - Duplicates data on each node. No Compositing.
// .SECTION Description
// vtkCaveRenderManager is like the tiled display render manager, but it
// uses arbitrary cameras.  A cave display is specified by
// the camera that points toward it.  The camera position is at the origin
// and the origin is where the person is initially standing.  The
// person can move to an arbitrary location, but the defining cameras
// are always positioned at the origin.
// This might replace vtkMultiDisplayManager in the future.

// This assumes data is duplicated on all nodes.  There is no compositing.

// .SECTION see also
// vtkRenderWindow vtkCompositeManager vtkMultiDisplayManager.

#ifndef __vtkCaveRenderManager_h
#define __vtkCaveRenderManager_h

#include "vtkParallelRenderManager.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkSocketController;
class vtkRenderer;
class vtkCamera;
class vtkTiledDisplaySchedule;
class vtkPVCompositeUtilities;
class vtkFloatArray;
class vtkUnsignedCharArray;
class vtkPVCaveClientInfo;
class vtkPVCompositeBuffer;

class VTK_EXPORT vtkCaveRenderManager : public vtkParallelRenderManager
{
public:
  static vtkCaveRenderManager *New();
  vtkTypeRevisionMacro(vtkCaveRenderManager,vtkParallelRenderManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Callbacks that initialize and finish the compositing.
  virtual void ClientEndRender();
  
  // Description:
  // If the user wants to handle the event loop, then they must call this
  // method to initialize the RMIs.
  virtual void InitializeRMIs();
  
  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  void SetController(vtkMultiProcessController* controller);

  // Description:
  // Set/Get the controller use to send final image to client
  void SetSocketController(vtkSocketController* controller);
  vtkGetObjectMacro(SocketController, vtkSocketController);

//BTX
  enum Tags {
    ROOT_RENDER_RMI_TAG=12721,
    SATELLITE_RENDER_RMI_TAG=12722,
    INFO_TAG=22135,
    DEFINE_DISPLAY_RMI_TAG = 89843,
    DEFINE_DISPLAY_INFO_TAG = 89844
  };
//ETX

  // Description:
  // Assumes one tile per process.  Call this on the client  multiple times
  // to define the tiles on the different processes.
  // "idx" is the index of the dsiplay/process to set.
  // origin, x, and y are in world coordinates.
  void DefineDisplay(int idx, double origin[3],
                     double x[3], double y[3]);
  void DefineDisplayRMI();

  // Description:
  // This is a hack to get around a shortcomming
  // of the SocketController.  There is no way to distinguish
  // between socket processes.
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);

  // Description:
  // Always uses the clients zbuffer value. (for picking).
  float GetZBufferValue(int x, int y);
  
  // Description:
  // Internal, but public for RMI/Callbacks.
  void ClientStartRender();
  void RootStartRenderRMI(vtkPVCaveClientInfo *info);
  void SatelliteStartRenderRMI();

protected:
  vtkCaveRenderManager();
  ~vtkCaveRenderManager();

  // Working toward general displays.
  void ComputeCamera(vtkPVCaveClientInfo *info, vtkCamera* cam);

  int ClientFlag;
  
  vtkSocketController* SocketController;

  void SetupCamera(int tileIdx, int reduction);

  unsigned long StartTag;
  unsigned long EndTag;
  
  void PreRenderProcessing() {}
  void PostRenderProcessing() {}
  void InternalSatelliteStartRender(vtkPVCaveClientInfo *info);
  
  // Definition of the display on this node.
  double DisplayOrigin[4];
  double DisplayX[4];
  double DisplayY[4];

private:
  vtkCaveRenderManager(const vtkCaveRenderManager&); // Not implemented
  void operator=(const vtkCaveRenderManager&); // Not implemented
};

#endif
