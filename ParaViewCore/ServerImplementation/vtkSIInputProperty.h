/*=========================================================================

  Program:   ParaView
  Module:    vtkSIInputProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIInputProperty
// .SECTION Description
// ServerSide Property use to set vtkOutputPort as method parameter.
// For that we need the object on which we should get the Port and its port
// number.

#ifndef __vtkSIInputProperty_h
#define __vtkSIInputProperty_h

#include "vtkSIProxyProperty.h"

class VTK_EXPORT vtkSIInputProperty : public vtkSIProxyProperty
{
public:
  static vtkSIInputProperty* New();
  vtkTypeMacro(vtkSIInputProperty, vtkSIProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls which input port this property uses when making connections.
  // By default, this is 0.
  vtkGetMacro(PortIndex, int);

//BTX
protected:
  vtkSIInputProperty();
  ~vtkSIInputProperty();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

  vtkSetMacro(PortIndex, int);
  int PortIndex;
private:
  vtkSIInputProperty(const vtkSIInputProperty&); // Not implemented
  void operator=(const vtkSIInputProperty&); // Not implemented
//ETX
};

#endif
