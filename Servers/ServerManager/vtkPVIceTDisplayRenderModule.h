/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTDisplayRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTDisplayRenderModule - Handles MPI and Client Server
// .SECTION Description
// This module hanldes both the cases of ParaView running Client-Server
// and ParaView running as a set MPI processes.

#ifndef __vtkPVIceTDisplayRenderModule_h
#define __vtkPVIceTDisplayRenderModule_h

#include "vtkPVLODRenderModule.h"

class vtkCompositeRenderManager;

class VTK_EXPORT vtkPVIceTDisplayRenderModule : public vtkPVLODRenderModule
{
public:
  static vtkPVIceTDisplayRenderModule* New();
  vtkTypeRevisionMacro(vtkPVIceTDisplayRenderModule,vtkPVLODRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProcessModule right after construction.
  virtual void SetProcessModule(vtkPVProcessModule *pm);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server).
  vtkSetMacro(ReductionFactor, int);
  vtkGetMacro(ReductionFactor, int);

  void StillRender();
  void InteractiveRender();

protected:
  vtkPVIceTDisplayRenderModule();
  ~vtkPVIceTDisplayRenderModule();

  vtkCompositeRenderManager* Composite;
  vtkClientServerID CompositeID;
  vtkClientServerID DisplayManagerID;

  int ReductionFactor;  

  vtkPVIceTDisplayRenderModule(const vtkPVIceTDisplayRenderModule&); // Not implemented
  void operator=(const vtkPVIceTDisplayRenderModule&); // Not implemented
};


#endif
