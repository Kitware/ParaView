/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateSuppressorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUpdateSuppressorProxy - proxy for an update suppressor
// .SECTION Description
// This proxy class is an example of how vtkSMProxy can be subclassed
// to add functionality. 

#ifndef __vtkSMUpdateSuppressorProxy_h
#define __vtkSMUpdateSuppressorProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMUpdateSuppressorProxy : public vtkSMSourceProxy
{
public:
  static vtkSMUpdateSuppressorProxy* New();
  vtkTypeRevisionMacro(vtkSMUpdateSuppressorProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);


protected:
  vtkSMUpdateSuppressorProxy();
  ~vtkSMUpdateSuppressorProxy();

private:
  vtkSMUpdateSuppressorProxy(const vtkSMUpdateSuppressorProxy&); // Not implemented
  void operator=(const vtkSMUpdateSuppressorProxy&); // Not implemented
};

#endif
