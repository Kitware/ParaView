// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDesktopDeliveryClient.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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

#include <vtkParallelRenderManager.h>

class VTK_EXPORT vtkDesktopDeliveryClient : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkDesktopDeliveryClient, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkDesktopDeliveryClient *New();

  // Description:
  // Needed for compatability.
  void SetUseCompositing(int v) {this->SetParallelRendering(v);}
  int GetUseCompositing() { return this->GetParallelRendering();}
  vtkBooleanMacro(UseCompositing, int);
  
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

  virtual void ComputeVisiblePropBounds(vtkRenderer *ren, float bounds[6]);

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

protected:
  vtkDesktopDeliveryClient();
  virtual ~vtkDesktopDeliveryClient();

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  int ServerProcessId;

  int ReplaceActors;

  // Updated by UpdateServerInfo.
  int RemoteDisplay;
  double RemoteImageProcessingTime;
  double TransferTime;

private:
  vtkDesktopDeliveryClient(const vtkDesktopDeliveryClient &); //Not implemented
  void operator=(const vtkDesktopDeliveryClient &);  //Not implemented
};


#endif //__vtkDesktopDeliveryClient_h

