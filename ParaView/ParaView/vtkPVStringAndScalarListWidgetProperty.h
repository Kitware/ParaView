/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringAndScalarListWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVStringAndScalarListWidgetProperty - a property for a list of strings and scalar values
// .SECTION Description
// vtkPVStringAndScalarListWidgetProperty is a subclass of
// vtkPVScalarListWidgetProperty that is used to pass both strings and scalar
// values to a VTK object.

#ifndef __vtkPVStringAndScalarListWidgetProperty_h
#define __vtkPVStringAndScalarListWidgetProperty_h

#include "vtkPVScalarListWidgetProperty.h"

class vtkStringList;

class VTK_EXPORT vtkPVStringAndScalarListWidgetProperty : public vtkPVScalarListWidgetProperty
{
public:
  static vtkPVStringAndScalarListWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVStringAndScalarListWidgetProperty,
                       vtkPVScalarListWidgetProperty);
  void PrintSelf(ostream &of, vtkIndent indent);
  
  // Description:
  // Pass values to VTK objects.
  virtual void AcceptInternal();
  
  // Description:
  // Set the method(s) to call on the specified VTK object.
  // numCmds is the number of methods to call.
  // cmds is a list of the methods.
  // numStrings is an array containing the number of string parameters needed
  // for each method.
  // numScalars is an array containing the number of scalar parameters needed
  // for each method.
  void SetVTKCommands(int numCmds, char **cmds, int *numStrings,
                      int *numScalars);

  // Description:
  // Specify the list of strings to pass to VTK.
  void SetStrings(int num, char **strings);
  
  // Description:
  // Add a string to the list of strings to pass to VTK.
  void AddString(char *string);
  
  // Description:
  // Set/get the string indicated by idx from the list of strings to pass
  // to VTK.
  void SetString(int idx, char *string);
  const char* GetString(int idx);
  
  // Description:
  // Get the total number of strings being sent to the specified VTK object.
  int GetNumberOfStrings();
  
protected:
  vtkPVStringAndScalarListWidgetProperty();
  ~vtkPVStringAndScalarListWidgetProperty();
  
  vtkStringList* Strings;
  int *NumberOfStringsPerCommand;
  
private:
  vtkPVStringAndScalarListWidgetProperty(const vtkPVStringAndScalarListWidgetProperty&); // Not implemented
  void operator=(const vtkPVStringAndScalarListWidgetProperty&); // Not implemented
};

#endif
