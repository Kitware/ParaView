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

#ifndef __vtkSMIceTDesktopRenderModuleProxy_h
#define __vtkSMIceTDesktopRenderModuleProxy_h

#include "vtkSMCompositeRenderModuleProxy.h"

class VTK_EXPORT vtkSMIceTDesktopRenderModuleProxy : public vtkSMCompositeRenderModuleProxy
{
public:
  static vtkSMIceTDesktopRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTDesktopRenderModuleProxy, vtkSMCompositeRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  // Control the RemoteDisplay property on vtkDesktopDeliveryServer.
  int RemoteDisplay;
 
  vtkSMProxy* DisplayManagerProxy;
private:
  vtkSMIceTDesktopRenderModuleProxy(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMIceTDesktopRenderModuleProxy&); // Not implemented.
};

#endif
