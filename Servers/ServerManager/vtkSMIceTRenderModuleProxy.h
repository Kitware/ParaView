/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTRenderModuleProxy - Multi Display using IceT.
// This can be used with or without tiles.
// When using without tiles.

#ifndef __vtkSMIceTRenderModuleProxy_h
#define __vtkSMIceTRenderModuleProxy_h

#include "vtkSMIceTDesktopRenderModuleProxy.h"

class VTK_EXPORT vtkSMIceTRenderModuleProxy : public vtkSMIceTDesktopRenderModuleProxy
{
public:
  static vtkSMIceTRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTRenderModuleProxy, vtkSMIceTDesktopRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Select the threshold at which any geometry is collected on the client.
  vtkSetMacro(CollectGeometryThreshold, double);
  vtkGetMacro(CollectGeometryThreshold, double);

protected:
  vtkSMIceTRenderModuleProxy();
  ~vtkSMIceTRenderModuleProxy();

  double CollectGeometryThreshold;

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();
  
  // Indicates if we should locally render.
  // Tile displays always locally render when using LOD (during Interactive Render).
  // Flag stillRender is set when this decision is to be made during StillRender
  // else it's 0 (for InteractiveRender);
  virtual int GetLocalRenderDecision(unsigned long totalMemory, int stillRender);

  virtual void InteractiveRender();
  virtual void StillRender();

  // Description:
  // Method called before/after Still Render is called.
  // Can be used to set GlobalLODFlag.
  virtual void BeginStillRender();
  virtual void EndStillRender();

  virtual void BeginInteractiveRender();
  virtual void EndInteractiveRender();

  // Description:
  // Indicates if geometry should be collected on the client.  If the data is
  // really big, sometimes even a decimated version of it does not fit on
  // the client well.
  int GetSuppressGeometryCollectionDecision();

  // Description:
  // Convenience method for synchronizing the SuppressGeometryCollection flag on
  // all the display proxies.
  void ChooseSuppressGeometryCollection();

private:
  vtkSMIceTRenderModuleProxy(const vtkSMIceTRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMIceTRenderModuleProxy&); // Not implemented.
};

#endif
