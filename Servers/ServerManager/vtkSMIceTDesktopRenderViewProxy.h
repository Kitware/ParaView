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
// .NAME vtkSMIceTDesktopRenderViewProxy
// .SECTION Description
// IceT Render View that can be used in client server configurations.
//
// This class does not use a MultiViewManager for multi-view configurations,
// instead, that reponsibilty is taken on by the RenderSyncManager.

#ifndef __vtkSMIceTDesktopRenderViewProxy_h
#define __vtkSMIceTDesktopRenderViewProxy_h

#include "vtkSMIceTCompositeViewProxy.h"

class VTK_EXPORT vtkSMIceTDesktopRenderViewProxy : public vtkSMIceTCompositeViewProxy
{
public:
  static vtkSMIceTDesktopRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTDesktopRenderViewProxy, vtkSMIceTCompositeViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to pass the GUISize to the RenderSyncManager.
  virtual void SetGUISize(int x, int y);

  // Description:
  // Overridden to pass the ViewPosition to the RenderSyncManager.
  virtual void SetViewPosition(int x, int y);

  // Description:
  // Set the view size.
  // Overridden to affect only the client-size render window size (server-side
  // sizes are managed by the RenderSyncManager).
  // Overrides superclasses implementation.
  virtual void SetViewSize(int width, int height);


  // Description:
  // Returns an image data that contains a "screenshot" of the window.
  // It is the responsibility of the caller to delete the image data.
  virtual vtkImageData* CaptureWindow(int magnification);

  // Description:
  // Get the value of the z buffer at a position. 
  // This is necessary for picking the center of rotation.
  virtual double GetZBufferValue(int x, int y);

//BTX
protected:
  vtkSMIceTDesktopRenderViewProxy();
  ~vtkSMIceTDesktopRenderViewProxy();

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
  // ImageReductionFactor needs to be set on the RenderSyncManager
  // rather than the ParallelRenderManager.
  virtual void SetImageReductionFactorInternal(int factor);

  // Description:
  // Overridden to pass the UseCompositing state to the RenderSyncManager rather
  // than the ParallelRenderManager.
  virtual void SetUseCompositing(bool usecompositing);

  // RenderManager managing client-server rendering.
  vtkSMProxy* RenderSyncManager;
  vtkClientServerID SharedServerRenderSyncManagerID;

private:
  vtkSMIceTDesktopRenderViewProxy(const vtkSMIceTDesktopRenderViewProxy&); // Not implemented
  void operator=(const vtkSMIceTDesktopRenderViewProxy&); // Not implemented
//ETX
};

#endif

