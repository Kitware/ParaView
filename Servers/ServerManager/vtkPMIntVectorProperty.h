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
// .NAME vtkPMIntVectorProperty
// .SECTION Description
//

#ifndef __vtkPMIntVectorProperty_h
#define __vtkPMIntVectorProperty_h

#include "vtkPMVectorProperty.h"

class VTK_EXPORT vtkPMIntVectorProperty : public vtkPMVectorProperty
{
public:
  static vtkPMIntVectorProperty* New();
  vtkTypeMacro(vtkPMIntVectorProperty, vtkPMVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If ArgumentIsArray is true, multiple elements are passed in as
  // array arguments. For example, For example, if
  // RepeatCommand is true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 1 2 3 4 5 6, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo array(1, 2)
  // * Invoke obj SetFoo array(3, 4)
  // * Invoke obj SetFoo array(5, 6)
  // @endverbatim
  vtkGetMacro(ArgumentIsArray, bool);

//BTX
protected:
  vtkPMIntVectorProperty();
  ~vtkPMIntVectorProperty();

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkPMProxy* proxyhelper, vtkPVXMLElement* element);

  bool Push(int* values, int number_of_elements);

  bool ArgumentIsArray;
private:
  vtkPMIntVectorProperty(const vtkPMIntVectorProperty&); // Not implemented
  void operator=(const vtkPMIntVectorProperty&); // Not implemented
//ETX
};

#endif
