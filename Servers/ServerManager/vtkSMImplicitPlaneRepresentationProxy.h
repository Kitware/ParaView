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
// .NAME vtkSMImplicitPlaneRepresentationProxy - proxy for a implicit plane representation
// .SECTION Description
// Specialized proxy for implicit planes. Overrides the default appearance
// of VTK implicit plane representation.

#ifndef __vtkSMImplicitPlaneRepresentationProxy_h
#define __vtkSMImplicitPlaneRepresentationProxy_h

#include "vtkSMWidgetRepresentationProxy.h"

class VTK_EXPORT vtkSMImplicitPlaneRepresentationProxy : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMImplicitPlaneRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMImplicitPlaneRepresentationProxy, vtkSMWidgetRepresentationProxy);
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

  virtual void SendRepresentation();

private:
  vtkSMImplicitPlaneRepresentationProxy(const vtkSMImplicitPlaneRepresentationProxy&); // Not implemented
  void operator=(const vtkSMImplicitPlaneRepresentationProxy&); // Not implemented
};

#endif
