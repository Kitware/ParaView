/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientCompositeManager.h
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
// .NAME vtkClientCompositeManager - Composites and sends image over socket.
// .SECTION Description
// vtkClientCompositeManager operates in client server mode.
// Server composites normaly.  I wanted to use vtkPVTreeComposite here,
// But have to rethink the architecture.
// Client receives the image over the socket.  It renders on top
// of the remote image. (No zbuffer).

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeManager.

#ifndef __vtkClientCompositeManager_h
#define __vtkClientCompositeManager_h

#include "vtkObject.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkSocketController;
class vtkCompositer;
class vtkRenderer;
class vtkDataArray;
class vtkFloatArray;

class VTK_EXPORT vtkClientCompositeManager : public vtkObject
{
public:
  static vtkClientCompositeManager *New();
  vtkTypeRevisionMacro(vtkClientCompositeManager,vtkObject);
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
  void ResetCamera(vtkRenderer *ren);
  void ResetCameraClippingRange(vtkRenderer *ren);
  void ComputeVisiblePropBounds(vtkRenderer *ren, 
                                float bounds[6]);
  void RenderRMI();
  void ComputeVisiblePropBoundsRMI();
  
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
  // since the socket controller allways thinks it is node 0.
  vtkSetMacro(ClientFlag, int);
  vtkGetMacro(ClientFlag, int);

  // Description:
  // This flag tells the compositer to use char values for pixel data rather than float.
  // Default is float.  I have seen some artifacts on some systems with char.
  void SetUseChar(int useChar);
  vtkGetMacro(UseChar, int);
  vtkBooleanMacro(UseChar, int);

  // Description:
  // This flag tells the compositier to get the color buffer as RGB 
  // instead of RGBA. We do not use the alpha value so it is not 
  // important to get.  ATI Radeon cards / drivers do not properly get 
  // the color buffer as RGBA.  This flag turns the UseChar flag on 
  // because VTK does not have methods to get the pixel data as RGB float.
  void SetUseRGB(int useRGB);
  vtkGetMacro(UseRGB, int);
  vtkBooleanMacro(UseRGB, int);

  // Description:
  // For values larger than 1, render a smaller image and display the
  // result using pixel replication.
  vtkSetMacro(ReductionFactor, int);
  vtkGetMacro(ReductionFactor, int);

  // Description:
  // Methods that are not used at the moment.
  virtual void SetRenderView(vtkObject*) {};

  // Description:
  // When the server has more than one process, this object
  // composites the buffers into one.  Defaults to vtkCompressCompositer.
  void SetCompositer(vtkCompositer *c);
  vtkGetObjectMacro(Compositer,vtkCompositer);

  // Description:
  // These parameters allow this object to manage a tiled display.
  // When the flag is on, the nodes (starting in lower left)
  // render tiles of a larger display defined by TiledDimensions.
  vtkSetMacro(Tiled,int);
  vtkGetMacro(Tiled,int);
  vtkBooleanMacro(Tiled,int);
  vtkSetVector2Macro(TiledDimensions,int);
  vtkGetVector2Macro(TiledDimensions,int);

//BTX
  enum Tags {
    RENDER_RMI_TAG=12721,
    WIN_INFO_TAG=22134,
    REN_INFO_TAG=22135
  };
//ETX

protected:
  vtkClientCompositeManager();
  ~vtkClientCompositeManager();
  
  vtkRenderWindow* RenderWindow;
  vtkMultiProcessController* CompositeController;
  vtkSocketController* ClientController;
  vtkCompositer *Compositer;

  int Tiled;
  int TiledDimensions[2];

  int ClientFlag;
  unsigned long StartTag;
  //unsigned long EndTag;
  unsigned long ResetCameraTag;
  unsigned long ResetCameraClippingRangeTag;
  
  void SetPDataSize(int x, int y);
  void ReallocPDataArrays();

  virtual void SatelliteStartRender();
  virtual void SatelliteEndRender();



  // Same method that is in vtkComposite manager.
  // We should find a way to shar this method. !!!!
  void MagnifyBuffer(vtkDataArray* localP, 
                     vtkDataArray* magP,
                     int windowSize[2]);

  // Our simple alternative to compositing.
  void ReceiveAndSetColorBuffer();

  vtkObject *RenderView;
  int ReductionFactor;

  vtkDataArray *PData;
  vtkFloatArray *ZData;
  // Temporary arrays used for compositing.
  vtkDataArray *PData2;
  vtkFloatArray *ZData2;

  int PDataSize[2];

  // This is used to hold the result from pixel replication.
  // It is only allocated on the client.
  vtkDataArray *MagnifiedPData;
  int MagnifiedPDataSize[2];

  int UseChar;
  int UseRGB;


private:
  vtkClientCompositeManager(const vtkClientCompositeManager&); // Not implemented
  void operator=(const vtkClientCompositeManager&); // Not implemented
};

#endif
