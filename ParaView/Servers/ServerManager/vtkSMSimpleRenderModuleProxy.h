/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleRenderModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleRenderModuleProxy - a simple render module.
// .SECTION Description
// A simple render module.

#ifndef __vtkSMSimpleRenderModuleProxy_h
#define __vtkSMSimpleRenderModuleProxy_h

#include "vtkSMRenderModuleProxy.h"
class vtkSMDisplayProxy;

class VTK_EXPORT vtkSMSimpleRenderModuleProxy : public vtkSMRenderModuleProxy
{
public:
  static vtkSMSimpleRenderModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMSimpleRenderModuleProxy, vtkSMRenderModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMSimpleRenderModuleProxy();
  ~vtkSMSimpleRenderModuleProxy();

private:
  vtkSMSimpleRenderModuleProxy(const vtkSMSimpleRenderModuleProxy&); // Not implemented.
  void operator=(const vtkSMSimpleRenderModuleProxy&); // Not implemented.
};


#endif

