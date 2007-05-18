/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTDesktopRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTDesktopRenderViewProxy - IceT render module, without
// tiles.
// Composite render module to support client/server compositing with one or
// more views. In multi-view mode, the client can have multiple render
// windows and the server has only one render window and one or more
// renderers for each render window the client has. The client has to tell
// the server the size of the render window and the position of each
// renderer. Each render module represents one render window (client)/
// renderers (server) pair.


#ifndef __vtkSMIceTDesktopRenderViewProxy_h
#define __vtkSMIceTDesktopRenderViewProxy_h

#include "vtkSMCompositeRenderViewProxy.h"

class vtkSMIceTDesktopRenderViewProxyProxySet;

class VTK_EXPORT vtkSMIceTDesktopRenderViewProxy : public vtkSMCompositeRenderViewProxy
{
public:
  static vtkSMIceTDesktopRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTDesktopRenderViewProxy, vtkSMCompositeRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set whether or not to order compositing.  If compositing is not ordered,
  // then the z buffer is used to composite.
  vtkGetMacro(DisableOrderedCompositing, int);
  vtkSetMacro(DisableOrderedCompositing, int);
  vtkBooleanMacro(DisableOrderedCompositing, int);

  virtual void AddDisplay(vtkSMAbstractDisplayProxy* disp);
  virtual void RemoveDisplay(vtkSMAbstractDisplayProxy* disp);

  virtual void StillRender();

  // Multi-view methods:

  // Description:
  // Set the proxy of the server display manager. This should be set
  // immediately after the render module is created. Setting this after
  // CreateVTKObjects() has been called has no effect.
  void SetServerDisplayManagerProxy(vtkSMProxy*);
  vtkGetObjectMacro(ServerDisplayManagerProxy, vtkSMProxy);

protected:
  vtkSMIceTDesktopRenderViewProxy();
  ~vtkSMIceTDesktopRenderViewProxy();

  // This method is the wierdest CreateVTKObjects I have known.
  // Basically we are trying to create the Renderer in a non-standard way.
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  int TileDimensions[2];
  int TileMullions[2];

  // Control the RemoteDisplay property on vtkDesktopDeliveryServer.
  int RemoteDisplay;
 
  int DisableOrderedCompositing;
  int OrderedCompositing;
  void SetOrderedCompositing(int oc);

  vtkSMProxy* DisplayManagerProxy;
  vtkSMProxy* PKdTreeProxy;

  // Generator is used only when volume rendering structured data.
  vtkSMProxy* PKdTreeGeneratorProxy;

  vtkSMProxy* ServerDisplayManagerProxy;

  // This flag is set when we generate the k-d tree ourselves 
  // (in case of structured volume rendering). 
  int UsingCustomKdTree;

  // Description:
  // The set of inputs (as proxies) to PKdTreeProxy that existed the last
  // time BuildLocator was called on PKdTreeProxy.
  vtkSMIceTDesktopRenderViewProxyProxySet *PartitionedData;

private:
  vtkSMIceTDesktopRenderViewProxy(const vtkSMIceTDesktopRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMIceTDesktopRenderViewProxy&); // Not implemented.
};

#endif
