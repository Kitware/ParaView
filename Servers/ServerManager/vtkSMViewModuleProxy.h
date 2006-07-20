/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewModuleProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMViewModuleProxy - a simple render module.
// .SECTION Description
// A simple render module.

#ifndef __vtkSMSimpleRenderModuleProxy_h
#define __vtkSMSimpleRenderModuleProxy_h

#include "vtkSMAbstractViewModuleProxy.h"

class VTK_EXPORT vtkSMViewModuleProxy : public vtkSMAbstractViewModuleProxy
{
public:
  static vtkSMViewModuleProxy* New();
  vtkTypeRevisionMacro(vtkSMViewModuleProxy, vtkSMAbstractViewModuleProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMViewModuleProxy();
  ~vtkSMViewModuleProxy();

private:
  vtkSMViewModuleProxy(const vtkSMViewModuleProxy&); // Not implemented.
  void operator=(const vtkSMViewModuleProxy&); // Not implemented.
};


#endif


