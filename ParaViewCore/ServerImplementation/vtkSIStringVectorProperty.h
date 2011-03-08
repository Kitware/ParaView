/*=========================================================================

  Program:   ParaView
  Module:    vtkSIStringVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIStringVectorProperty
// .SECTION Description
// ServerImplementation Property to deal with String array as method arguments.

#ifndef __vtkSIStringVectorProperty_h
#define __vtkSIStringVectorProperty_h

#include "vtkSIVectorProperty.h"

class VTK_EXPORT vtkSIStringVectorProperty : public vtkSIVectorProperty
{
public:
  static vtkSIStringVectorProperty* New();
  vtkTypeMacro(vtkSIStringVectorProperty, vtkSIVectorProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIStringVectorProperty();
  ~vtkSIStringVectorProperty();

  enum ElementTypes{ INT, DOUBLE, STRING };

  // Description:
  // Push a new state to the underneath implementation
  virtual bool Push(vtkSMMessage*, int);

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

  // Description:
  // Parse the xml for the property.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element);

private:
  vtkSIStringVectorProperty(const vtkSIStringVectorProperty&); // Not implemented
  void operator=(const vtkSIStringVectorProperty&); // Not implemented

  class vtkVectorOfStrings;
  class vtkVectorOfInts;

  bool Push(const vtkVectorOfStrings &values);
  vtkVectorOfInts* ElementTypes;
//ETX
};

#endif
