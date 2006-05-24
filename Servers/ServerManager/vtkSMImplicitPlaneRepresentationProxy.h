/*=========================================================================

  Program:   ParaView
  Module:    vtkSMImplicitPlaneRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMImplicitPlaneRepresentationProxy - proxy for a VTK lookup table
// .SECTION Description
// This proxy class is an example of how vtkSMProxy can be subclassed
// to add functionality. It adds one simple method : Build().

#ifndef __vtkSMImplicitPlaneRepresentationProxy_h
#define __vtkSMImplicitPlaneRepresentationProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMImplicitPlaneRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMImplicitPlaneRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMImplicitPlaneRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given the number of objects (numObjects), class name 
  // (VTKClassName) and server ids ( this->GetServerIDs()), 
  // this methods instantiates the objects on the server(s)
  // This method is overridden to change the default appearance.
  virtual void CreateVTKObjects(int numObjects);

protected:
  vtkSMImplicitPlaneRepresentationProxy();
  ~vtkSMImplicitPlaneRepresentationProxy();

private:
  vtkSMImplicitPlaneRepresentationProxy(const vtkSMImplicitPlaneRepresentationProxy&); // Not implemented
  void operator=(const vtkSMImplicitPlaneRepresentationProxy&); // Not implemented
};

#endif
