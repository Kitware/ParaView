/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNullProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMNullProxy - proxy with stands for NULL object on the server.
// .SECTION Description
// vtkSMNullProxy stands for a 0 on the server side.

#ifndef __vtkSMNullProxy_h
#define __vtkSMNullProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMNullProxy : public vtkSMProxy
{
public:
  static vtkSMNullProxy* New();
  vtkTypeMacro(vtkSMNullProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMNullProxy();
  ~vtkSMNullProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMNullProxy(const vtkSMNullProxy&); // Not implemented.
  void operator=(const vtkSMNullProxy&); // Not implemented.
};

#endif
