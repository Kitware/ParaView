/*=========================================================================

  Program:   ParaView
  Module:    vtkDesktopDeliveryClient.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDesktopDeliveryClient - An object for remote rendering.
// .SECTION Description
// The two vtkDesktopDelivery objects (vtkDesktopDeliveryClient and
// vtkDesktopDeliveryServer) work together to enable interactive viewing of
// remotely rendered data.  The client attaches itself to a vtkRenderWindow
// and, optionally, a vtkRenderWindowInteractor.  Whenever a new rendering
// is requested, the client alerts the server, the server renders a new
// frame, and ships the image back to the client, which will display the
// image in the vtkRenderWindow.
// .SECTION note
// You should set up the renderers and render window interactor before setting
// the render window.  We set up observers on the renderer, and we have no
// easy way of knowing when the renderers change.
// .SECTION see also
// vtkDesktopDeliveryServer vtkMultiProcessController vtkRenderWindow
// vtkRenderWindowInteractor

#ifndef __vtkDesktopDeliveryClient_h
#define __vtkDesktopDeliveryClient_h

#include "vtkParallelRenderManager.h"

class vtkCommand;

class VTK_EXPORT vtkDesktopDeliveryClient : public vtkParallelRenderManager
{
public:
  vtkTypeMacro(vtkDesktopDeliveryClient, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkDesktopDeliveryClient *New();

  // Description:
  // Generally, when you turn compositing off, you expect the image in the
  // local render window to remain unchanged.  If you are doing remote
  // display, then there is no point in rendering anything on the server
  // side.  Thus, if RemoteDisplay is on and UseCompositing is turned off,
  // then ParallelRendering is also turned off altogether.  Likewise,
  // ParallelRendering is turned back on when UseCompositing is turned on.
  virtual void SetUseCompositing(int v);

  // Description:
  // Set/Get the controller that is attached to a vtkDesktopDeliveryServer.
  // This object will assume that the controller has two processors, and
  // that the controller on the opposite side of the controller has been
  // given to the server object.
  virtual void SetController(vtkMultiProcessController *controller);

  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // If ReplaceActors is set to on (the default), then all the actors of
  // each Renderer attached to the RenderWindow are replaced with a single
  // bounding box.  The replacement occurs whenever a camera is reset or
  // the visible prop bounds are calculated.  If set to off, the actors are
  // never modified.
  vtkSetMacro(ReplaceActors, int);
  vtkGetMacro(ReplaceActors, int);
  vtkBooleanMacro(ReplaceActors, int);

  virtual void ComputeVisiblePropBounds(vtkRenderer *ren, double bounds[6]);

  // Description:
  // Returns true if remote display is on.  If remote display is on, then
  // the RenderWindow will be updated with an image rendered on the client.
  // If not, the RenderWindow keeps the image its Renderers draw.  The
  // RemoteDisplay flag is determined by the server.  The remote display
  // is specified by the server, so the value may be out of date if an
  // image has not been rendered since the last time the value changed on
  // the server.
  vtkGetMacro(RemoteDisplay, int);

  vtkGetMacro(RemoteImageProcessingTime, double);
  vtkGetMacro(TransferTime, double);
  virtual double GetRenderTime() {
    return (this->RenderTime - this->RemoteImageProcessingTime);
  }
  virtual double GetImageProcessingTime() {
    return (  this->RemoteImageProcessingTime
        + this->TransferTime + this->ImageProcessingTime);
  }

  // For ParaView
  void SetSquirtLevel (int l)
  { 
    if (l == 0)
      {
      this->SquirtOff();
      }
    else
      {
      this->SquirtOn(); 
      this->SetSquirtCompressionLevel(l-1);
      }
  }

  // Description:
  // Enables or disables SQUIRT compression for image delivery.  By
  // default, compression is off.  Note that this function may be replaced
  // with a more universal image compression at a later date.
  vtkGetMacro(Squirt, int);
  vtkSetMacro(Squirt, int);
  vtkBooleanMacro(Squirt, int);

  // Description:
  // Sets the compression level used by SQUIRT.  Higher values result in
  // better compression but lower resolution in the color space (the size
  // of the image is unaffected by this option).
  vtkGetMacro(SquirtCompressionLevel, int);
  vtkSetClampMacro(SquirtCompressionLevel, int, 0, 5);

  virtual void SetImageReductionFactorForUpdateRate(double desiredUpdateRate);
  float GetZBufferValue(int x, int y);

  // Description:
  // For internal use.
  virtual void ReceiveImageFromServer();

protected:
  vtkDesktopDeliveryClient();
  ~vtkDesktopDeliveryClient();

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  int ServerProcessId;

  int ReplaceActors;

  // Updated by UpdateServerInfo.
  int RemoteDisplay;
  double RemoteImageProcessingTime;
  double TransferTime;

  // Add to the stream information required for window synchronization on each
  // render.
  virtual void CollectWindowInformation(vtkMultiProcessStream&);

  // Squirt options (probably to be replaced later).
  int Squirt;
  int SquirtCompressionLevel;
  vtkUnsignedCharArray *SquirtBuffer;

  void SquirtDecompress(vtkUnsignedCharArray *in, vtkUnsignedCharArray *out);

  int UseCompositing;

  int ReceivedImageFromServer;
  vtkCommand *ReceiveImageCallback;
  
private:
  vtkDesktopDeliveryClient(const vtkDesktopDeliveryClient &); //Not implemented
  void operator=(const vtkDesktopDeliveryClient &); //Not implemented
};

#endif

