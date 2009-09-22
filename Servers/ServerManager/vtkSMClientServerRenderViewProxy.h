/*=========================================================================

  Program:   ParaView
  Module:    vtkSMClientServerRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientServerRenderViewProxy
// .SECTION Description
// Render View that can be used in client server configurations.
// Note that this class mirrors the API of vtkSMIceTDesktopRenderViewProxy.


#ifndef __vtkSMClientServerRenderViewProxy_h
#define __vtkSMClientServerRenderViewProxy_h

#include "vtkSMMultiProcessRenderView.h"

class VTK_EXPORT vtkSMClientServerRenderViewProxy : public vtkSMMultiProcessRenderView
{
public:
  static vtkSMClientServerRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMClientServerRenderViewProxy, vtkSMMultiProcessRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to pass the GUISize to the RenderSyncManager.
  virtual void SetGUISize(int x, int y);

  // Description:
  // Overridden to pass the ViewPosition to the RenderSyncManager.
  virtual void SetViewPosition(int x, int y);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

//BTX
protected:
  vtkSMClientServerRenderViewProxy();
  ~vtkSMClientServerRenderViewProxy();

  // Description:
  // Pre-CreateVTKObjects initialization.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Post-CreateVTKObjects initialization.
  virtual void EndCreateVTKObjects();

  // Description:
  // Overridden to disable lossy compression during still render.
  virtual void BeginStillRender();

  // Description:
  // Overridden to enable lossy compression during interactive render.
  virtual void BeginInteractiveRender();

  // Description:
  // In multiview setups, some viewmodules may share certain objects with each
  // other. This method is used in such cases to give such views an opportunity
  // to share those objects.
  // Default implementation is empty.
  virtual void InitializeForMultiView(vtkSMViewProxy* otherView);

  // Description:
  // Initialize the RenderSyncManager properties. Called in
  // EndCreateVTKObjects().
  virtual void InitializeRenderSyncManager();

  // Description:
  // Overridden to pass the UseCompositing state to the RenderSyncManager rather
  // than the ParallelRenderManager.
  virtual void SetUseCompositing(bool usecompositing);

  // RenderManager managing client-server rendering.
  vtkSMProxy* RenderSyncManager;
  vtkClientServerID SharedServerRenderSyncManagerID;

  // ID used to identify renders with the MultiViewManager.
  int RenderersID;

private:
  vtkSMClientServerRenderViewProxy(const vtkSMClientServerRenderViewProxy&); // Not implemented
  void operator=(const vtkSMClientServerRenderViewProxy&); // Not implemented
//ETX
};

#endif

