/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiDisplayManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiDisplayManager - Duplicates data on each node.
// .SECTION Description
// vtkMultiDisplayManager operates in multiple processes.  Each process
// (except 0) is responsible for rendering to one tile of a large display.
// Process 0 is reserved for interaction and directing the view of the 
// large display.

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeManager.

#ifndef __vtkMultiDisplayManager_h
#define __vtkMultiDisplayManager_h

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
class vtkPVMultiDisplayInfo;
class vtkPVCompositeBuffer;

class VTK_EXPORT vtkMultiDisplayManager : public vtkParallelRenderManager
{
public:
  static vtkMultiDisplayManager *New();
  vtkTypeRevisionMacro(vtkMultiDisplayManager,vtkParallelRenderManager);
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

  // Description:
  // Set/Get dimensions (in number of displays) of the
  // display array.  There has to be NxM+1 processes.
  vtkSetVector2Macro(TileDimensions, int);
  vtkGetVector2Macro(TileDimensions, int);

  // Description:
  // Convience methods for accessing buffer variables 
  // in composite utilities object.
  unsigned long GetMaximumMemoryUsage();
  void SetMaximumMemoryUsage(unsigned long mem);
  unsigned long GetTotalMemoryUsage();

//BTX
  enum Tags {
    ROOT_RENDER_RMI_TAG=12721,
    SATELLITE_RENDER_RMI_TAG=12722,
    INFO_TAG=22135
  };
//ETX

  //============================================================
  // New compositing methods.

  void InitializeSchedule();

  // Description:
  // This enables and disables the 
  // use of active pixel compression.
  vtkSetMacro(UseCompositeCompression, int);
  vtkGetMacro(UseCompositeCompression, int);
  vtkBooleanMacro(UseCompositeCompression, int);

  // Description:
  // This value is only used for interactive rendering.
  // Reduction factor = 1 means normal (full sized) rendering
  // and compositing.  When ReductionFactor > 1, a small window
  // is rendered (subsampled) and composited.
  void SetImageReductionFactor(double f) {this->SetLODReductionFactor((int)f);}
  vtkSetMacro(LODReductionFactor, int);
  vtkGetMacro(LODReductionFactor, int);

  // Description:
  // Just used for debugging.
  vtkGetObjectMacro(CompositeUtilities,vtkPVCompositeUtilities);

  // Description:
  // Working toward general displays.
  void ComputeCamera(float *o, float *x, float *y,
                     float *p, vtkCamera* cam);

  // Description:
  // This is a hack to get around a shortcomming
  // of the SocketController.  There is no way to distinguish
  // between socket processes.
  vtkSetMacro(ClientFlag,int);
  vtkGetMacro(ClientFlag,int);

  // Description:
  // A bad API !!!!  This flag is set when MPI node 0 is the client.
  // This works in combination with controllers and ClientFlag.
  vtkSetMacro(ZeroEmpty,int);
  vtkGetMacro(ZeroEmpty,int);

  // Description:
  // Always uses the clients zbuffer value. (for picking).
  float GetZBufferValue(int x, int y);
  
  // Description:
  // Internal, but public for RMI/Callbacks.
//BTX
  void ClientStartRender();
  void RootStartRender(vtkPVMultiDisplayInfo info);
  void SatelliteStartRender();

//ETX
protected:
  vtkMultiDisplayManager();
  ~vtkMultiDisplayManager();

  int ClientFlag;
  
  vtkSocketController* SocketController;

  // For managing buffers.
  vtkPVCompositeUtilities* CompositeUtilities;

  // Abstracting buffer storage, so we can rander on demand
  // and free buffers as soon as possible.
  vtkPVCompositeBuffer** TileBuffers;
  int TileBufferArrayLength;
  // Gets the stored buffer.  Renders if necessary.
  vtkPVCompositeBuffer* GetTileBuffer(int tileId);
  // Does refernce counting properly.
  void SetTileBuffer(int tileIdx, vtkPVCompositeBuffer* buf);
  // Length 0 frees the array.
  void InitializeTileBuffers(int length);

  void SetupCamera(int tileIdx, int reduction);


  int ImageReductionFactor;
  int LODReductionFactor;

  unsigned long StartTag;
  unsigned long EndTag;
  
  // Any processes can display the tiles.
  // There can be more processes than tiles.
  int TileDimensions[2];
  int NumberOfProcesses;

  vtkTiledDisplaySchedule* Schedule;
  int ZeroEmpty;

  int UseCompositeCompression;

  void Composite();

  void PreRenderProcessing() {}
  void PostRenderProcessing() {}
  void InternalSatelliteStartRender(vtkPVMultiDisplayInfo info);
  
private:
  vtkMultiDisplayManager(const vtkMultiDisplayManager&); // Not implemented
  void operator=(const vtkMultiDisplayManager&); // Not implemented
};

#endif
