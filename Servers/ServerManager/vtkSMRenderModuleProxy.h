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
  vtkRenderWindowInteractor* GetInteractor();
  vtkRenderWindow* GetRenderWindow();
  vtkRenderer* GetRenderer();
  vtkRenderer* GetRenderer2D();

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
  
  vtkSMProxy* RendererProxy;
  vtkSMProxy* Renderer2DProxy;
  vtkSMProxy* ActiveCameraProxy;
  vtkSMProxy* RenderWindowProxy;
  vtkSMProxy* InteractorProxy;

  vtkGetObjectMacro(RendererProxy, vtkSMProxy);
  vtkGetObjectMacro(Renderer2DProxy, vtkSMProxy);
  vtkGetObjectMacro(ActiveCameraProxy, vtkSMProxy);
  vtkGetObjectMacro(InteractorProxy, vtkSMProxy);

  //BTX
  friend class vtkSMSimpleDisplayProxy;
  friend class vtkSMAxesProxy;
  friend class vtkSM3DWidgetProxy;
  friend class vtkSMScalarBarWidgetProxy;
  friend class vtkSMCubeAxesDisplayProxy;
  friend class vtkSMXYPlotDisplayProxy;
  friend class vtkSMInteractorObserverProxy;
  friend class vtkSMPointLabelDisplayProxy;
  
  
  friend class vtkPVCameraIcon; //needs access to Camera proxy.
  //ETX
  // Just a convienience method.
  int GetDisplayVisibility(vtkSMDisplayProxy* pDisp);

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

