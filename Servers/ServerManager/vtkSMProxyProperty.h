/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyProperty -
// .SECTION Description

#ifndef __vtkSMProxyProperty_h
#define __vtkSMProxyProperty_h

#include "vtkSMProperty.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMProxyProperty : public vtkSMProperty
{
public:
  static vtkSMProxyProperty* New();
  vtkTypeRevisionMacro(vtkSMProxyProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  void SetProxy(vtkSMProxy* proxy);

  // Description:
  vtkGetObjectMacro(Proxy, vtkSMProxy);

protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty();

  //BTX
  // Description:
  // Update the vtk object (with the given id and on the given
  // nodes) with the property values(s).
  virtual void AppendCommandToStream(
    vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  vtkSMProxy* Proxy;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&); // Not implemented
  void operator=(const vtkSMProxyProperty&); // Not implemented
};

#endif
