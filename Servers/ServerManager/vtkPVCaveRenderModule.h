/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCaveRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCaveRenderModule - Handles Cave with duplication.
// .SECTION Description
// This module hanldes rendering to a cave.  I am trying to configure
// arbitrary displays as a variation of the tiled display modules.
// This module only handles dulication with no compositing.

#ifndef __vtkPVCaveRenderModule_h
#define __vtkPVCaveRenderModule_h

#include "vtkPVCompositeRenderModule.h"


class VTK_EXPORT vtkPVCaveRenderModule : public vtkPVCompositeRenderModule
{
public:
  static vtkPVCaveRenderModule* New();
  vtkTypeRevisionMacro(vtkPVCaveRenderModule,vtkPVCompositeRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProcessModule right after construction.
  virtual void SetProcessModule(vtkPVProcessModule *pm);

  // Description:
  // Always collect when using LOD
  virtual void InteractiveRender();
  virtual void StillRender();

protected:
  vtkPVCaveRenderModule();
  ~vtkPVCaveRenderModule();

  virtual vtkPVPartDisplay* CreatePartDisplay();
  void LoadConfigurationFile(int numDisplays);

  vtkPVCaveRenderModule(const vtkPVCaveRenderModule&); // Not implemented
  void operator=(const vtkPVCaveRenderModule&); // Not implemented
};


#endif
