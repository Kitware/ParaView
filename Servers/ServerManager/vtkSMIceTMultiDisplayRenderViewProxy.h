/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTMultiDisplayRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTMultiDisplayRenderViewProxy - multi display using IceT.
// .SECTION Description
// vtkSMIceTMultiDisplayRenderViewProxy is the view proxy used when using tile
// displays.

#ifndef __vtkSMIceTMultiDisplayRenderViewProxy_h
#define __vtkSMIceTMultiDisplayRenderViewProxy_h

#include "vtkSMIceTDesktopRenderViewProxy.h"

class VTK_EXPORT vtkSMIceTMultiDisplayRenderViewProxy : 
  public vtkSMIceTDesktopRenderViewProxy
{
public:
  static vtkSMIceTMultiDisplayRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTMultiDisplayRenderViewProxy, 
    vtkSMIceTDesktopRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Select the threshold at which any geometry is collected on the client.
  vtkSetMacro(CollectGeometryThreshold, double);
  vtkGetMacro(CollectGeometryThreshold, double);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server) when still rendering.
  vtkSetMacro(StillReductionFactor, int);
  vtkGetMacro(StillReductionFactor, int);

protected:
  vtkSMIceTMultiDisplayRenderViewProxy();
  ~vtkSMIceTMultiDisplayRenderViewProxy();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects();


  double CollectGeometryThreshold;
  int StillReductionFactor;

private:
  vtkSMIceTMultiDisplayRenderViewProxy(const vtkSMIceTMultiDisplayRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMIceTMultiDisplayRenderViewProxy&); // Not implemented.
};

#endif

