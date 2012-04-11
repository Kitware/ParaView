/*=========================================================================

  Program:   ParaView
  Module:    vtkSMVectorProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMVectorProperty - abstract superclass for all vector properties
// .SECTION Description
// vtkSMVectorProperty defines an interface common to all vector properties
// as well as some common settings. A vector property contains a list
// of values passed to one or more invocations of a command. How the
// values are distributed to the different invocations is controlled
// by several parameters.

#ifndef __vtkSMVectorProperty_h
#define __vtkSMVectorProperty_h

#include "vtkSMProperty.h"

class VTK_EXPORT vtkSMVectorProperty : public vtkSMProperty
{
public:
  vtkTypeMacro(vtkSMVectorProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the size of the vector.
  virtual unsigned int GetNumberOfElements() = 0;

  // Description:
  // Sets the size of the vector.
  virtual void SetNumberOfElements(unsigned int num) = 0;

  virtual unsigned int GetNumberOfUncheckedElements() { return 0; }
  virtual void SetNumberOfUncheckedElements(unsigned int num) { (void) num; }

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
  vtkGetMacro(RepeatCommand, int);
  vtkSetMacro(RepeatCommand, int);
  vtkBooleanMacro(RepeatCommand, int);

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
  vtkSetMacro(NumberOfElementsPerCommand, int);

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
  vtkGetMacro(UseIndex, int);
  vtkSetMacro(UseIndex, int);
  vtkBooleanMacro(UseIndex, int);

  // Description:
  // Command that can be used to remove all values. 
  // Typically used when RepeatCommand = 1. If set, the clean command
  // is called before the main Command.
  vtkSetStringMacro(CleanCommand);
  vtkGetStringMacro(CleanCommand);

  // Description: 
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

  // Description:
  // If SetNumberCommand is set, it is called before Command
  // with the number of arguments as the parameter.
  vtkSetStringMacro(SetNumberCommand);
  vtkGetStringMacro(SetNumberCommand);

  virtual void ClearUncheckedElements();

  char* SetNumberCommand;
  
  vtkSetStringMacro(InitialString);
  vtkGetStringMacro(InitialString);


protected:
  vtkSMVectorProperty();
  ~vtkSMVectorProperty();

  // Description:
  // Generic implementation that use SetElementAsString to push the value the
  // property
  virtual int LoadState( vtkPVXMLElement* element, vtkSMProxyLocator* loader);
  // Must be overriden if you want to use the default implementation
  // of LoadState
  virtual int SetElementAsString(int idx, const char* value) = 0;

  int RepeatCommand;
  int NumberOfElementsPerCommand;
  int UseIndex;

  char* CleanCommand;
  
  char* InitialString;

  // Description:
  // Set the appropriate ivars from the xml element.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

private:
  vtkSMVectorProperty(const vtkSMVectorProperty&); // Not implemented
  void operator=(const vtkSMVectorProperty&); // Not implemented
};

#endif
