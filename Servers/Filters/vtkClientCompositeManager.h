/*=========================================================================

  Program:   ParaView
  Module:    vtkClientCompositeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientCompositeManager - Composites and sends image over socket.
// .SECTION Description
// vtkClientCompositeManager operates in client server mode.
// Server composites normaly.  I wanted to use vtkPVTreeComposite here,
// But have to rethink the architecture.
// Only the first render in the render window gets composited.
// Client receives the image over the socket.  Additional renderers 
// render on top of the remote image. (No zbuffer).

// .SECTION see also
// vtkMultiProcessController vtkRenderWindow vtkCompositeManager.

#ifndef __vtkClientCompositeManager_h
#define __vtkClientCompositeManager_h

#include "vtkParallelRenderManager.h"

class vtkRenderWindow;
class vtkMultiProcessController;
class vtkSocketController;
class vtkCompositer;
class vtkRenderer;
class vtkDataArray;
class vtkFloatArray;
class vtkUnsignedCharArray;
class vtkImageData;
class vtkImageActor;
class vtkCamera;

class VTK_EXPORT vtkClientCompositeManager : public vtkParallelRenderManager
{
public:
  static vtkClientCompositeManager *New();
  vtkTypeRevisionMacro(vtkClientCompositeManager,vtkParallelRenderManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the RenderWindow to use for compositing.
  // We add a start and end observer to the window.
  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // Used to get satellite windows rendering off screen.
  virtual void InitializeOffScreen();

  // Description:
  // Callbacks that initialize and finish the compositing.
  virtual void StartRender();
  virtual void EndRender();
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
  void SetController(vtkMultiProcessController* controller);

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
  // This flag tells the compositier to get the color buffer as RGB 
  // instead of RGBA. We do not use the alpha value so it is not 
  // important to get.  ATI Radeon cards / drivers do not properly get 
  // the color buffer as RGBA.  
  void SetUseRGB(int useRGB);
  vtkGetMacro(UseRGB, int);
  vtkBooleanMacro(UseRGB, int);

  // Description:
  // For values larger than 1, render a smaller image and display the
  // result using pixel replication.
  vtkSetMacro(ImageReductionFactor, int);

  // Description:
  // When the server has more than one process, this object
  // composites the buffers into one.  Defaults to vtkCompressCompositer.
  void SetCompositer(vtkCompositer *c);
  vtkGetObjectMacro(Compositer,vtkCompositer);

//BTX
  enum Tags {
    RENDER_RMI_TAG=12721,
    WIN_INFO_TAG=22134,
    REN_INFO_TAG=22135,
    GATHER_Z_RMI_TAG=987987,
    SERVER_Z_TAG=88771,
    CLIENT_Z_TAG=88772
  };
//ETX

  // Description:
  // Get the z buffer value at a pixel.  GatherZBufferValue is
  // an internal method.
  float GetZBufferValue(int x, int y);
  void GatherZBufferValueRMI(int x, int y);

  // Description:
  // Turn on and off Squirt compression.
  // Level 0 means no compression.
  vtkSetClampMacro(SquirtLevel, int, 0, 7);
  vtkGetMacro(SquirtLevel, int);

protected:
  vtkClientCompositeManager();
  ~vtkClientCompositeManager();
  
  vtkSocketController* ClientController;
  vtkCompositer *Compositer;

  int ClientFlag;
  unsigned long StartTag;
  
  void SetPDataSize(int x, int y);
  void ReallocPDataArrays();

  virtual void SatelliteStartRender();
  virtual void SatelliteEndRender();

  void PreRenderProcessing() {}
  void PostRenderProcessing() {}
  vtkImageData *CompositeData;
  vtkImageActor *ImageActor;
  // This is used to restore the camera.
  // We have to change it to display the image actor.
  vtkCamera* SavedCamera;

  // Same method that is in vtkComposite manager.
  // We should find a way to share this method. !!!!
  void MagnifyBuffer(vtkDataArray* localP, 
                     vtkDataArray* magP,
                     int windowSize[2]);

  void DoubleBuffer(vtkDataArray* localP, 
                    vtkDataArray* magP,
                    int windowSize[2]);


  // Our simple alternative to compositing.
  void ReceiveAndSetColorBuffer();

  vtkObject *RenderView;
  double InternalReductionFactor;

  int PDataSize[2];
  vtkDataArray *PData;
  vtkFloatArray *ZData;
  // Temporary arrays used for compositing.
  vtkDataArray *PData2;
  vtkFloatArray *ZData2;

  int SquirtLevel;
  vtkUnsignedCharArray *SquirtArray;
  void SquirtCompress(vtkUnsignedCharArray *in,
                      vtkUnsignedCharArray *out,
                      int compress_level);
  void SquirtDecompress(vtkUnsignedCharArray *in,
                        vtkUnsignedCharArray *out);

  vtkUnsignedCharArray *BaseArray;
  void DeltaEncode(vtkUnsignedCharArray *buf);
  void DeltaDecode(vtkUnsignedCharArray *buf);

  int UseRGB;

private:
  vtkClientCompositeManager(const vtkClientCompositeManager&); // Not implemented
  void operator=(const vtkClientCompositeManager&); // Not implemented
};

#endif
