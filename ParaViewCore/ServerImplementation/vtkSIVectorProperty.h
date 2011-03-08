/*=========================================================================

  Program:   ParaView
  Module:    vtkSIVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIVectorProperty
// .SECTION Description
// Abstract class for SIProperty that hold an array of values.
// Define the array management API

#ifndef __vtkSIVectorProperty_h
#define __vtkSIVectorProperty_h

#include "vtkSIProperty.h"

class VTK_EXPORT vtkSIVectorProperty : public vtkSIProperty
{
public:
  vtkTypeMacro(vtkSIVectorProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If RepeatCommand is true, the command is invoked multiple times,
  // each time with NumberOfElementsPerCommand values. For example, if
  // RepeatCommand is true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 1 2 3 4 5 6, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo 1 2
  // * Invoke obj SetFoo 3 4
  // * Invoke obj SetFoo 5 6
  // @endverbatim
  vtkGetMacro(NumberOfElementsPerCommand, int);

  // Description:
  // If UseIndex and RepeatCommand are true, the property will add
  // an index integer before each command. For example, if UseIndex and
  // RepeatCommand are true, NumberOfElementsPerCommand is 2, the
  // command is SetFoo and the values are 5 6 7 8 9 10, the resulting
  // stream will have:
  // @verbatim
  // * Invoke obj SetFoo 0 5 6
  // * Invoke obj SetFoo 1 7 8
  // * Invoke obj SetFoo 2 9 10
  // @endverbatim
  vtkGetMacro(UseIndex, bool);

  // Description:
  // Command that can be used to remove all values.
  // Typically used when RepeatCommand = 1. If set, the clean command
  // is called before the main Command.
  vtkGetStringMacro(CleanCommand);

  // Description:
  // If SetNumberCommand is set, it is called before Command
  // with the number of arguments as the parameter.
  vtkGetStringMacro(SetNumberCommand);


//BTX
protected:
  vtkSIVectorProperty();
  ~vtkSIVectorProperty();
  vtkSetStringMacro(CleanCommand);
  vtkSetStringMacro(SetNumberCommand);

  char* SetNumberCommand;
  char* CleanCommand;
  bool UseIndex;
  int NumberOfElementsPerCommand;

  // Description:
  // Set the appropriate ivars from the xml element.
  virtual bool ReadXMLAttributes(vtkSIProxy* proxyhelper,
    vtkPVXMLElement* element);

private:
  vtkSIVectorProperty(const vtkSIVectorProperty&); // Not implemented
  void operator=(const vtkSIVectorProperty&); // Not implemented
//ETX
};

#endif
