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
/**
 * @class   vtkSIVectorProperty
 *
 * Abstract class for SIProperty that hold an array of values.
 * Define the array management API
*/

#ifndef vtkSIVectorProperty_h
#define vtkSIVectorProperty_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSIVectorProperty : public vtkSIProperty
{
public:
  vtkTypeMacro(vtkSIVectorProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * If RepeatCommand is true, the command is invoked multiple times,
   * each time with NumberOfElementsPerCommand values. For example, if
   * RepeatCommand is true, NumberOfElementsPerCommand is 2, the
   * command is SetFoo and the values are 1 2 3 4 5 6, the resulting
   * stream will have:
   * @verbatim
   * * Invoke obj SetFoo 1 2
   * * Invoke obj SetFoo 3 4
   * * Invoke obj SetFoo 5 6
   * @endverbatim
   */
  vtkGetMacro(NumberOfElementsPerCommand, int);
  //@}

  //@{
  /**
   * If UseIndex and RepeatCommand are true, the property will add
   * an index integer before each command. For example, if UseIndex and
   * RepeatCommand are true, NumberOfElementsPerCommand is 2, the
   * command is SetFoo and the values are 5 6 7 8 9 10, the resulting
   * stream will have:
   * @verbatim
   * * Invoke obj SetFoo 0 5 6
   * * Invoke obj SetFoo 1 7 8
   * * Invoke obj SetFoo 2 9 10
   * @endverbatim
   */
  vtkGetMacro(UseIndex, bool);
  //@}

  //@{
  /**
   * Command that can be used to remove all values.
   * Typically used when RepeatCommand = 1. If set, the clean command
   * is called before the main Command.
   */
  vtkGetStringMacro(CleanCommand);
  //@}

  //@{
  /**
   * If SetNumberCommand is set, it is called before Command
   * with the number of arguments as the parameter.
   */
  vtkGetStringMacro(SetNumberCommand);
  //@}

  vtkGetStringMacro(InitialString);

protected:
  vtkSIVectorProperty();
  ~vtkSIVectorProperty() override;
  vtkSetStringMacro(CleanCommand);
  vtkSetStringMacro(SetNumberCommand);
  vtkSetStringMacro(InitialString);

  char* SetNumberCommand;
  char* CleanCommand;
  bool UseIndex;
  int NumberOfElementsPerCommand;
  char* InitialString;

  /**
   * Set the appropriate ivars from the xml element.
   */
  bool ReadXMLAttributes(vtkSIProxy* proxyhelper, vtkPVXMLElement* element) override;

private:
  vtkSIVectorProperty(const vtkSIVectorProperty&) = delete;
  void operator=(const vtkSIVectorProperty&) = delete;
};

#endif
