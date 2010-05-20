/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCaveRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCaveRenderViewProxy - multi display using IceT.
// .SECTION Description
// vtkSMCaveRenderViewProxy is the view proxy used when using tile
// displays.

#ifndef __vtkSMCaveRenderViewProxy_h
#define __vtkSMCaveRenderViewProxy_h

#include "vtkSMIceTMultiDisplayRenderViewProxy.h"

class VTK_EXPORT vtkSMCaveRenderViewProxy : 
  public vtkSMIceTMultiDisplayRenderViewProxy
{
public:
  static vtkSMCaveRenderViewProxy* New();
  vtkTypeMacro(vtkSMCaveRenderViewProxy,
    vtkSMIceTMultiDisplayRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMCaveRenderViewProxy();
  ~vtkSMCaveRenderViewProxy();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects();

  // Description:
  // Setup the CaveRenderManager based on the displays set in the 
  // server information
  void ConfigureRenderManagerFromServerInformation();

private:
  vtkSMCaveRenderViewProxy(const vtkSMCaveRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMCaveRenderViewProxy&); // Not implemented.

//ETX
};

#endif

