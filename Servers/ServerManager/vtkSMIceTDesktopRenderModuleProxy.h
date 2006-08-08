/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTDesktopRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTDesktopRenderModuleProxy - IceT render module, without
// tiles.
// Composite render module to support client/server compositing with one or
// more views. In multi-view mode, the client can have multiple render
// windows and the server has only one render window and one or more
// renderers for each render window the client has. The client has to tell
// the server the size of the render window and the position of each
// renderer. Each render module represents one render window (client)/
// renderers (server) pair.


#ifndef __vtkSMIceTDesktopRenderModuleProxy_h
#define __vtkSMIceTDesktopRenderModuleProxy_h

#include "vtkSMCompositeRenderModuleProxy.h"

class vtkSMIceTDesktopRenderModuleProxyProxySet;

class VTK_EXPORT vtkSMIceTDesktopRenderModuleProxy : public vtkSMCompositeRenderModuleProxy
{
public:
  static vtkSMIceTDesktopRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTDesktopRenderModuleProxy, vtkSMCompositeRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set whether or not to order compositing.  If compositing is not ordered,
  // then the z buffer is used to composite.
  vtkGetMacro(OrderedCompositing, int);
  virtual void SetOrderedCompositing(int);
  vtkBooleanMacro(OrderedCompositing, int);

  virtual void AddDisplay(vtkSMAbstractDisplayProxy* disp);

  virtual void StillRender();

  // Multi-view methods:

  // Description:
  // Sets the size of the server render window.
  void SetGUISize(int x, int y);

  // Description:
  // Sets the position of the view associated with this module inside
  // the server render window. (0,0) corresponds to upper left corner.
  void SetWindowPosition(int x, int y);

  // Description:
  // Set the proxy of the server render window. This should be set
  // immediately after the render module is created. Setting this
  // after CreateVTKObjects() has been called has no effect.
  void SetServerRenderWindowProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerRenderWindowProxy, vtkSMProxy);

  // Description:
  // Set the proxy of the server composite manager. This should be set
  // immediately after the render module is created. Setting this after
  // CreateVTKObjects() has been called has no effect.
  void SetServerCompositeManagerProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerCompositeManagerProxy, vtkSMProxy);

  // Description:
  // Set the proxy of the server display manager. This should be set
  // immediately after the render module is created. Setting this after
  // CreateVTKObjects() has been called has no effect.
  void SetServerDisplayManagerProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerDisplayManagerProxy, vtkSMProxy);

  // Description:
  // Set the Id of the render module. This is only needed for multi view
  // and has to be unique. It is used to match client/server render RMIs
  vtkSetMacro(RenderModuleId, int);
  vtkGetMacro(RenderModuleId, int);

protected:
  vtkSMIceTDesktopRenderModuleProxy();
  ~vtkSMIceTDesktopRenderModuleProxy();

  // This method is the wierdest CreateVTKObjects I have known.
  // Basically we are trying to create the Renderer in a non-standard way.
  virtual void CreateVTKObjects(int numObjects);
 
  // Description:
  // Subclasses must decide what type of CompositeManagerProxy they need.
  // This method is called to make that decision. Subclasses are expected to
  // add the CompositeManagerProxy as a SubProxy named "CompositeManager".
  virtual void CreateCompositeManager();

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  int TileDimensions[2];
  int TileMullions[2];

  // Control the RemoteDisplay property on vtkDesktopDeliveryServer.
  int RemoteDisplay;
 
  int OrderedCompositing;

  vtkSMProxy* DisplayManagerProxy;
  vtkSMProxy* PKdTreeProxy;

  vtkSMProxy* ServerRenderWindowProxy;
  vtkSMProxy* ServerCompositeManagerProxy;
  vtkSMProxy* ServerDisplayManagerProxy;

  int RenderModuleId;

  // Description:
  // The set of inputs (as proxies) to PKdTreeProxy that existed the last
  // time BuildLocator was called on PKdTreeProxy.
  vtkSMIceTDesktopRenderModuleProxyProxySet *PartitionedData;

private:
  vtkSMIceTDesktopRenderModuleProxy(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
};

#endif
