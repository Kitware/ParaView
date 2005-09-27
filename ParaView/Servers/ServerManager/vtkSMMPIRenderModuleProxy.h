/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMPIRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMPIRenderModuleProxy - Handle MPI and Client Server.
// .SECTION Description
// This module hanldes both the cases of ParaView running Client-Server
// and ParaView running as a set MPI processes.

#ifndef __vtkSMMPIRenderModuleProxy_h
#define __vtkSMMPIRenderModuleProxy_h

#include "vtkSMCompositeRenderModuleProxy.h"

class VTK_EXPORT vtkSMMPIRenderModuleProxy : public vtkSMCompositeRenderModuleProxy
{
public:
  static vtkSMMPIRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMMPIRenderModuleProxy, vtkSMCompositeRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compression uses active pixel encoding of color and zbuffer.
  virtual void SetUseCompositeCompression(int val);
  
protected:
  vtkSMMPIRenderModuleProxy();
  ~vtkSMMPIRenderModuleProxy();

  // Description:
  // Subclasses must decide what type of CompositeManagerProxy they need.
  // This method is called to make that decision. Subclasses are expected to
  // add the CompositeManagerProxy as a SubProxy named "CompositeManager".
  virtual void CreateCompositeManager();

  // Description:
  // Subclasses should override this method to intialize the Composite Manager.
  // This is called after CreateVTKObjects();
  virtual void InitializeCompositingPipeline();

  void SetCompositer(const char* proxyname);

private:
  vtkSMMPIRenderModuleProxy(const vtkSMMPIRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMMPIRenderModuleProxy&); // Not implemented.
};

#endif

