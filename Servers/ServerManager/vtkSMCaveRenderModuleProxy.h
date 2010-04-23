/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCaveRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCaveRenderModuleProxy - Handles Cave with duplication.
// .SECTION Description
// This module hanldes rendering to a cave.  I am trying to configure
// arbitrary displays as a variation of the tiled display modules.
// This module only handles dulication with no compositing.

#ifndef __vtkSMCaveRenderModuleProxy_h
#define __vtkSMCaveRenderModuleProxy_h

#include "vtkSMCompositeRenderModuleProxy.h"

class vtkPVServerOptionsInternals;

class VTK_EXPORT vtkSMCaveRenderModuleProxy : public vtkSMCompositeRenderModuleProxy
{
public:
  static vtkSMCaveRenderModuleProxy* New();
  vtkTypeMacro(vtkSMCaveRenderModuleProxy, vtkSMCompositeRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMCaveRenderModuleProxy();
  ~vtkSMCaveRenderModuleProxy();
  
  // Description:
  // Subclasses must decide what type of RenderSyncManagerProxy they need.
  // This method is called to make that decision. Subclasses are expected to
  // add the RenderSyncManagerProxy as a SubProxy named "RenderSyncManager".
  virtual void CreateRenderSyncManager();

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  void LoadConfigurationFile(int numDisplays);
  void ConfigureFromServerInformation();

  // Description:
  // For Cave composition is always collect.
  virtual int GetLocalRenderDecision(unsigned long, int);
private:
  vtkSMCaveRenderModuleProxy(const vtkSMCaveRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMCaveRenderModuleProxy&); // Not implemented.

  vtkPVServerOptionsInternals* CaveInternals;
};

#endif

