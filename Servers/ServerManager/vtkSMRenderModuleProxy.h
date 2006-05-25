/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderModuleProxy - Manages rendering and displaying data.
// .SECTION Description
// This is an abstract super class for all rendering modules.
// Provides very basic functionality.
#ifndef __vtkSMRenderModuleProxy_h
#define __vtkSMRenderModuleProxy_h

#include "vtkSMProxy.h"

class vtkCamera;
class vtkCollection;
class vtkImageData;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkRenderer;
class vtkPVRenderModuleHelper;
class vtkSMDisplay;
class vtkSMDisplayProxy;
class vtkPVClientServerIdCollectionInformation;

// TODO: have to change the PVCameraManipulators to do ResetCamera on
// the RenderModule rather than renderer.
class VTK_EXPORT vtkSMRenderModuleProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMRenderModuleProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Any display that must be rendered by this rendermodule
  // needs to be added to it. 
  // NOTE: If you call this method directly (without using properties)
  // don't forget to call UpdateVTKObjects() on the RenderModule.
  virtual void AddDisplay(vtkSMDisplayProxy* disp);
  virtual void RemoveDisplay(vtkSMDisplayProxy* disp);

  // Description:
  // Removes all added displays. 
  // (Calls RemoveFromRenderModule on all displays).
  virtual void RemoveAllDisplays();

  // Description:
  // Renders using Still/FullRes or interactive/LODs
  virtual void StillRender();
  virtual void InteractiveRender();
  
  // Description
  // Subclass can create their own vtkSMDisplayProxy object by
  // implementing this method.
  // So far, others displays are not.
  virtual vtkSMDisplayProxy* CreateDisplayProxy();

  // Description:
  // Update the cache of all visible part displays. For flip books.
  virtual void CacheUpdate(int idx, int total);

  // Description:
  // Calls InvalidateGeometry() on all part displays. For flip books.
  virtual void InvalidateAllGeometries();

  // Description:
  // Callback for the triangle strips check button
  void SetUseTriangleStrips(int val);
  
  // Description:
  // Callback for the immediate mode rendering check button
  void SetUseImmediateMode(int val);
   
  // Description:
  // Access to the rendering-related objects for the GUI.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(Renderer2D, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);

  // Description:
  // Convenience method to set the background color.
  void SetBackgroundColorCM(double rgb[3]);

  // Description:
  // Reset the camera clipping range.
  void ResetCameraClippingRange();

  // Description:
  // Controls whether the render module invokes abort check events.
  vtkSetMacro(RenderInterruptsEnabled,int);
  vtkGetMacro(RenderInterruptsEnabled,int);
  vtkBooleanMacro(RenderInterruptsEnabled,int);

  // Description:
  // Compute the bounding box of all visible props.
  void ComputeVisiblePropBounds(double bounds[6]);

  // Description:
  // Returns the display collection.
  vtkGetObjectMacro(Displays, vtkCollection);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

  // Description:
  // Performs a pick in the selected screen area and returns a list
  // of ClientServerIds for the DisplayProxies hit.
  vtkPVClientServerIdCollectionInformation* 
    Pick(int xs, int ys, int xe, int ye);

  // Description:
  // Reset camera to the given bounds.
  void ResetCamera(double bds[6]);

  // Description:
  // Reset camera. Interally calls ComputeVisiblePropBounds
  // to obtain the bounds.
  void ResetCamera();
  
  // Description:
  // Save the display in batch script. This will eventually get 
  // removed as we will generate batch script from ServerManager
  // state. However, until then.
  virtual void SaveInBatchScript(ofstream* file);

  // Description:
  // This method calls UpdateInformation on the Camera Proxy
  // and sets the Camera properties according to the info
  // properties.
  void SynchronizeCameraProperties();

  // Description:
  // Internal method do not call directly. 
  // Synchornizes the properties of the 2D and 3D renders before
  // each render.
  void SynchronizeRenderers();

  // Description:
  // Generate a screenshot from the render window.
  virtual int WriteImage(const char* filename, const char* writerName);

  // Description:
  // Enable/Disable the LightKit.
  void SetUseLight(int enable);

  // Description:
  // Returns the dimensions of the first node of the render server (in
  // the argument size). Returns 1 on success, 0 on failure.
  int GetServerRenderWindowSize(int size[2]);

  // Description:
  // Called when saving server manager state.
  // Overridden to SynchronizeCameraProperties before saving the properties.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);

  // Description:
  // Indicates if we should locally render.
  virtual int IsRenderLocal() { return 1; }

  // Description:
  // Update all visible displays (therefore sources)
  virtual void UpdateAllDisplays();  

  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);

protected:
  vtkSMRenderModuleProxy();
  ~vtkSMRenderModuleProxy();

  // Description:
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Overridden since Interactor properties must be cleared.
  void UnRegisterVTKObjects();

  // Description:
  // Set the LOD decision.
  void SetLODFlag(int val);

  // This collection keeps a reference to all Display Proxies added
  // to this module.
  vtkCollection* Displays;

  // Collection of props added to the renderer.
  vtkCollection* RendererProps; 

  // Collection of props added to the renderer2D.
  vtkCollection* Renderer2DProps;
 
  vtkSMProxy* RendererProxy;
  vtkSMProxy* Renderer2DProxy;
  vtkSMProxy* ActiveCameraProxy;
  vtkSMProxy* RenderWindowProxy;
  vtkSMProxy* InteractorProxy;
  vtkSMProxy* LightKitProxy;
  vtkSMProxy* LightProxy;
  vtkSMProxy* HelperProxy;

  vtkGetObjectMacro(RendererProxy, vtkSMProxy);
  vtkGetObjectMacro(Renderer2DProxy, vtkSMProxy);
  vtkGetObjectMacro(ActiveCameraProxy, vtkSMProxy);
  vtkGetObjectMacro(InteractorProxy, vtkSMProxy);
  vtkGetObjectMacro(LightKitProxy, vtkSMProxy);
  vtkGetObjectMacro(LightProxy, vtkSMProxy);
  vtkGetObjectMacro(HelperProxy, vtkSMProxy);

  // Pointer to client side objects,
  // for convienience.
  vtkRenderer* Renderer2D;
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkRenderWindowInteractor* Interactor;
  vtkCamera* ActiveCamera;
  vtkPVRenderModuleHelper* Helper;
  

  // Description:
  // Methods called by Display proxies to add/remove the
  // actor proxies to appropriate renderer.
  // Note: these methods affect the reference count
  // of the added/removed actor proxy.
  virtual void AddPropToRenderer(vtkSMProxy* proxy);
  virtual void AddPropToRenderer2D(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer(vtkSMProxy* proxy);
  virtual void RemovePropFromRenderer2D(vtkSMProxy* proxy);
  
  //BTX
  friend class vtkSMDisplayProxy;
  friend class vtkSMNew3DWidgetProxy;
  //ETX

  // This is the XMLName of the proxy to get created when CreateDisplayProxy
  // is called. It must be a proxy belonging to the group "displays"
  // and must be a subclass of vtkSMDisplayProxy.
  char* DisplayXMLName;
  vtkSetStringMacro(DisplayXMLName);

  int RenderInterruptsEnabled;
  int ResetCameraClippingRangeTag;
  int AbortCheckTag;
  int StartRenderEventTag;

  int UseTriangleStrips;
  int UseImmediateMode;

  // Description:
  // Method called before/after Still Render is called.
  // Can be used to set GlobalLODFlag.
  virtual void BeginStillRender();
  virtual void EndStillRender();

  virtual void BeginInteractiveRender();
  virtual void EndInteractiveRender();
  
  

private:
  vtkSMRenderModuleProxy(const vtkSMRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMRenderModuleProxy&); // Not implemented.
};


#endif

