/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiDisplayRenderModule - Handles MPI and Client Server
// .SECTION Description
// This module hanldes both the cases of ParaView running Client-Server
// and ParaView running as a set MPI processes.

#ifndef __vtkPVMultiDisplayRenderModule_h
#define __vtkPVMultiDisplayRenderModule_h

#include "vtkPVCompositeRenderModule.h"


class VTK_EXPORT vtkPVMultiDisplayRenderModule : public vtkPVCompositeRenderModule
{
public:
  static vtkPVMultiDisplayRenderModule* New();
  vtkTypeRevisionMacro(vtkPVMultiDisplayRenderModule,vtkPVCompositeRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProcessModule right after construction.
  virtual void SetProcessModule(vtkProcessModule *pm);

  // Description:
  // Always collect when using LOD
  virtual void InteractiveRender();
  virtual void StillRender();

  // Description:
  // Whether to use compression or not.
  virtual void SetUseCompositeCompression(int val);

protected:
  vtkPVMultiDisplayRenderModule();
  ~vtkPVMultiDisplayRenderModule();

  virtual vtkSMPartDisplay* CreatePartDisplay();
  int UseCompositeCompression;

  vtkPVMultiDisplayRenderModule(const vtkPVMultiDisplayRenderModule&); // Not implemented
  void operator=(const vtkPVMultiDisplayRenderModule&); // Not implemented
};


#endif
