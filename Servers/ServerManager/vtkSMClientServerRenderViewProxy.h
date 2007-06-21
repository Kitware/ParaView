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
// Note that this class mirrors the API of vtkSMIceTDesktopRenderViewProxy. The
// implementaion (i.e. cxx file) for both the classes is infact the same.


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
  // Squirt is a hybrid run length encoding and bit reduction compression
  // algorithm that is used to compress images for transmition from the
  // server to client.  Value of 0 disabled all compression.  Level zero is just
  // run length compression with no bit compression (lossless).
  // Squirt compression is only used for client-server image delivery during
  // InteractiveRender.
  vtkSetClampMacro(SquirtLevel, int, 0, 7);
  vtkGetMacro(SquirtLevel, int);

  // Description:
  // Overridden to pass the GUISize to the RenderSyncManager.
  virtual void SetGUISize(int x, int y);

  // Description:
  // Overridden to pass the ViewPosition to the RenderSyncManager.
  virtual void SetViewPosition(int x, int y);

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
  // Overridden to disable squirt compression.
  virtual void BeginStillRender();

  // Description:
  // Overridden to use user-specified squirt compression.
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

  // Description:
  // Internal method to set the squirt level on the RenderSyncManager.
  void SetSquirtLevelInternal(int level);

  // RenderManager managing client-server rendering.
  vtkSMProxy* RenderSyncManager;
  vtkClientServerID SharedServerRenderSyncManagerID;

  int SquirtLevel;

private:
  vtkSMClientServerRenderViewProxy(const vtkSMClientServerRenderViewProxy&); // Not implemented
  void operator=(const vtkSMClientServerRenderViewProxy&); // Not implemented
//ETX
};

#endif

