/*=========================================================================

  Program:   ParaView
  Module:    vtkIceTClientCompositeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIceTClientCompositeManager - Composites and sends image over socket.
// .SECTION Description
// vtkIceTClientCompositeManager operates in client server mode.
// Server composites normaly.  I wanted to use vtkPVTreeComposite here,
// But have to rethink the architecture.
// Client receives the image over the socket.  It renders on top
// of the remote image. (No zbuffer).

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkPVTreeComposite

#ifndef __vtkIceTClientCompositeManager_h
#define __vtkIceTClientCompositeManager_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkSocketController;
class vtkCompositer;
class vtkRenderer;
class vtkDataArray;
class vtkFloatArray;
class vtkUnsignedCharArray;
class vtkIceTRenderManager;

class VTK_EXPORT vtkIceTClientCompositeManager : public vtkObject
{
public:
  static vtkIceTClientCompositeManager *New();
  vtkTypeRevisionMacro(vtkIceTClientCompositeManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Used to get satellite windows rendering off screen.
  void InitializeOffScreen();

  // Description:
  // Callbacks that initialize and finish the compositing.
  virtual void StartRender();
  void RenderRMI();
  
  // Description:
  // If the user wants to handle the event loop, then they must call this
  // method to initialize the RMIs.
  virtual void InitializeRMIs();
  
  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  void SetCompositeController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(CompositeController, vtkMultiProcessController);

  // Description:
  // Set/Get the controller use to communicate to client.
  void SetClientController(vtkSocketController* controller);
  vtkGetObjectMacro(ClientController, vtkSocketController); 

  // Description:
  // This flag is needed to differentiate between client and server,
  // since the socket controller always thinks it is node 0.
  vtkSetMacro(ClientFlag, int);
  vtkGetMacro(ClientFlag, int);

  // Description:
  // For values larger than 1, render a smaller image and display the
  // result using pixel replication.
  vtkSetMacro(ImageReductionFactor, int);
  vtkGetMacro(ImageReductionFactor, int);

  // Description:
  // Methods that are not used at the moment.
  virtual void SetRenderView(vtkObject*) {};

  // Description:
  // This is the server (IceT) manager.
  // This is the way we propagate the parameters client->server.
  void SetIceTManager(vtkIceTRenderManager *c);
  vtkGetObjectMacro(IceTManager,vtkIceTRenderManager);

  // Description:
  // These parameters allow this object to manage a tiled display.
  // When the flag is on, the nodes (starting in lower left)
  // render tiles of a larger display defined by TiledDimensions.
  vtkSetMacro(Tiled,int);
  vtkGetMacro(Tiled,int);
  vtkBooleanMacro(Tiled,int);
  vtkSetVector2Macro(TiledDimensions,int);
  vtkGetVector2Macro(TiledDimensions,int);

  
  // Description:
  // This parameter is used when user want to specify explicitely the size of
  // each tile display. Useful for testing.
  vtkSetVector2Macro(TileSize,int);
  vtkGetVector2Macro(TileSize,int);

//BTX
  enum Tags {
    RENDER_RMI_TAG   = 12721,
    WIN_INFO_TAG     = 22134,
    REN_INFO_TAG     = 22135,
    GATHER_Z_RMI_TAG = 987987,
    SERVER_Z_TAG     = 88771,
    CLIENT_Z_TAG     = 88772
  };
//ETX

  // Description:
  // Switch between local client rendering and distributed compositing.
  vtkSetMacro(UseCompositing, int);
  vtkGetMacro(UseCompositing, int);
  vtkBooleanMacro(UseCompositing, int);

  // Description:
  // Get the z buffer value at a pixel.  GatherZBufferValue is
  // an internal method.
  float GetZBufferValue(int x, int y);
  void GatherZBufferValueRMI(int x, int y);

  // Description:
  // This is not used.  It is here until we can clean up
  // the render module superclasses.
  vtkSetMacro(UseCompositeCompression,int);
  vtkGetMacro(UseCompositeCompression,int);
  vtkBooleanMacro(UseCompositeCompression,int);

protected:
  vtkIceTClientCompositeManager();
  ~vtkIceTClientCompositeManager();
  
  vtkRenderWindow* RenderWindow;
  vtkMultiProcessController* CompositeController;
  vtkSocketController* ClientController;
  vtkIceTRenderManager *IceTManager;

  int Tiled;
  int TiledDimensions[2];
  int TileSize[2];  //dimension of each tile display

  int ClientFlag;
  unsigned long StartTag;
  //unsigned long EndTag;
  
  virtual void SatelliteStartRender();
  virtual void SatelliteEndRender();

  vtkObject *RenderView;
  int ImageReductionFactor;

  int UseCompositing;
  int UseCompositeCompression;

//BTX
  enum {
    ACKNOWLEDGE_RMI = 17231
  };
//ETX

private:
  vtkIceTClientCompositeManager(const vtkIceTClientCompositeManager&); // Not implemented
  void operator=(const vtkIceTClientCompositeManager&); // Not implemented
};

#endif
