/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMInputProperty
// .SECTION Description
//

#ifndef __vtkPMInputProperty_h
#define __vtkPMInputProperty_h

#include "vtkPMProxyProperty.h"

class VTK_EXPORT vtkPMInputProperty : public vtkPMProxyProperty
{
public:
  static vtkPMInputProperty* New();
  vtkTypeMacro(vtkPMInputProperty, vtkPMProxyProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls which input port this property uses when making connections.
  // By default, this is 0.
  vtkGetMacro(PortIndex, int);

//BTX
protected:
  vtkPMInputProperty();
  ~vtkPMInputProperty();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

  vtkSetMacro(PortIndex, int);
  int PortIndex;
private:
  vtkPMInputProperty(const vtkPMInputProperty&); // Not implemented
  void operator=(const vtkPMInputProperty&); // Not implemented
//ETX
};

#endif
