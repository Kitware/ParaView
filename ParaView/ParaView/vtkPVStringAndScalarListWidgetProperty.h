/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVStringAndScalarListWidgetProperty.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
