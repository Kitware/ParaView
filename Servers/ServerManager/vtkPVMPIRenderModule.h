/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMPIRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMPIRenderModule - Handles MPI and Client Server
// .SECTION Description
// This module hanldes both the cases of ParaView running Client-Server
// and ParaView running as a set MPI processes.

#ifndef __vtkPVMPIRenderModule_h
#define __vtkPVMPIRenderModule_h

#include "vtkPVCompositeRenderModule.h"


class VTK_EXPORT vtkPVMPIRenderModule : public vtkPVCompositeRenderModule
{
public:
  static vtkPVMPIRenderModule* New();
  vtkTypeRevisionMacro(vtkPVMPIRenderModule,vtkPVCompositeRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProcessModule right after construction.
  virtual void SetProcessModule(vtkProcessModule *pm);

  // Description:
  // Compression uses active pixel encoding of color and zbuffer.
  virtual void SetUseCompositeCompression(int val);
  
protected:
  vtkPVMPIRenderModule();
  ~vtkPVMPIRenderModule();

  vtkPVMPIRenderModule(const vtkPVMPIRenderModule&); // Not implemented
  void operator=(const vtkPVMPIRenderModule&); // Not implemented
};


#endif
