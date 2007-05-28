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
  vtkSMIceTDesktopRenderModuleProxy();
  ~vtkSMIceTDesktopRenderModuleProxy();

  // This method is the weirdest CreateVTKObjects I have known.
  // Basically we are trying to create the Renderer in a non-standard way.
  virtual void CreateVTKObjects();

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
  vtkSMIceTDesktopRenderModuleProxyProxySet *PartitionedData;

private:
  vtkSMIceTDesktopRenderModuleProxy(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
};

#endif
