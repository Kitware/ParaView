/*=========================================================================

  Program:   ParaView
  Module:    vtkPVIceTRenderModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVIceTRenderModule - Handles MPI and Client Server
// .SECTION Description
// This module hanldes both the cases of ParaView running Client-Server
// and ParaView running as a set MPI processes.

#ifndef __vtkPVIceTRenderModule_h
#define __vtkPVIceTRenderModule_h

#include "vtkPVCompositeRenderModule.h"

class vtkCompositeRenderManager;

class VTK_EXPORT vtkPVIceTRenderModule : public vtkPVCompositeRenderModule
{
public:
  static vtkPVIceTRenderModule* New();
  vtkTypeRevisionMacro(vtkPVIceTRenderModule,vtkPVCompositeRenderModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ProcessModule right after construction.
  virtual void SetProcessModule(vtkProcessModule *pm);

  // Description:
  // IceT Does not have this option.
  void SetUseCompositeCompression(int);

protected:
  vtkPVIceTRenderModule();
  ~vtkPVIceTRenderModule();

  // This is the IceT manager that is ignorent of the client.
  // It runs on all processes of the server.
  vtkClientServerID DisplayManagerID;

  vtkPVIceTRenderModule(const vtkPVIceTRenderModule&); // Not implemented
  void operator=(const vtkPVIceTRenderModule&); // Not implemented
};


#endif
