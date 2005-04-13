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
class vtkCollection;
class vtkSMDisplay;
class vtkSMDisplayProxy;
class vtkRenderWindowInteractor;
class vtkRenderWindow;
class vtkRenderer;
class vtkCamera;

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
  // Must find a way to avoid exposing this.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(Renderer2D, vtkRenderer);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);

  // Description:
  // Convenience method to set the background color.
  void SetBackgroundColor(double rgb[3]);

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
  // Reset camera to the given bounds.
  void ResetCamera(double bds[6]);
  
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
  // Generate a screenshot from the render window.
  // Mostly here for batch mode testing.
  void WriteImage(const char* filename, const char* writerName);

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
  // This is used before a render to make sure all visible sources
  // have been updated.
  virtual void UpdateAllDisplays();
  
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

  vtkGetObjectMacro(RendererProxy, vtkSMProxy);
  vtkGetObjectMacro(Renderer2DProxy, vtkSMProxy);
  vtkGetObjectMacro(ActiveCameraProxy, vtkSMProxy);
  vtkGetObjectMacro(InteractorProxy, vtkSMProxy);

  // Pointer to client side objects,
  // for convienience.
  vtkRenderer* Renderer2D;
  vtkRenderer* Renderer;
  vtkRenderWindow* RenderWindow;
  vtkRenderWindowInteractor* Interactor;
  vtkCamera* ActiveCamera;
  

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
  friend class vtkSMSimpleDisplayProxy;
  friend class vtkSMAxesProxy;
  friend class vtkSM3DWidgetProxy;
  friend class vtkSMScalarBarWidgetProxy;
  friend class vtkSMCubeAxesDisplayProxy;
  friend class vtkSMXYPlotDisplayProxy;
  friend class vtkSMInteractorObserverProxy;
  friend class vtkSMPointLabelDisplayProxy;
  
  //ETX
  // This is the XMLName of the proxy to get created when CreateDisplayProxy
  // is called. It must be a proxy belonging to the group "displays"
  // and must be a subclass of vtkSMDisplayProxy.
  char* DisplayXMLName;
  vtkSetStringMacro(DisplayXMLName);

  int RenderInterruptsEnabled;
  int ResetCameraClippingRangeTag;
  int AbortCheckTag;

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

