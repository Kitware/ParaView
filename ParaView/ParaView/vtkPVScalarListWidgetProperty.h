/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarListWidgetProperty.h
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
// .NAME vtkPVScalarListWidgetProperty - a property for a list of scalar values
// .SECTION Description
// vtkPVScalarListWidgetProperty is a subclass of vtkPVWidgetProperty that is
// used to pass a list of scalar values to a VTK object.  There can be
// multiple VTK commands used to pass these values to VTK.

#ifndef __vtkPVScalarListWidgetProperty_h
#define __vtkPVScalarListWidgetProperty_h

#include "vtkPVWidgetProperty.h"

class VTK_EXPORT vtkPVScalarListWidgetProperty : public vtkPVWidgetProperty
{
public:
  static vtkPVScalarListWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVScalarListWidgetProperty, vtkPVWidgetProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Pass values to VTK objects.
  virtual void AcceptInternal();
  
  // Description:
  // Set the method(s) to call on the specified VTK object.
  // numCmds is the number of methods to call.
  // cmds is a list of the methods.
  // numScalars is an array containing the number of scalar parameters needed
  // for each method.
  void SetVTKCommands(int numCmds, char **cmds, int *numScalars);

  // Description:
  // Specify the list of scalars to pass to VTK.
  void SetScalars(int num, float *scalars);
  
  // Description:
  // Add a scalar to the list of scalars to pass to VTK.
  void AddScalar(float scalar);
  
  // Description:
  // Return the list of scalar values to pass to VTK.
  float* GetScalars() { return this->Scalars; }
  
  // Description:
  // Set/get the scalar value indicated by idx from the list of scalar values
  // to pass to VTK.
  void SetScalar(int idx, float scalar);
  float GetScalar(int idx);

  // Description:
  // Set the total number of scalars being sent to the specified VTK
  // object.
  vtkGetMacro(NumberOfScalars, int);

  // Description:
  // Set the animation time for this property.  This sets the modified flag on
  // the widget, and then calls Reset on it.
  virtual void SetAnimationTime(float time);
  
protected:
  vtkPVScalarListWidgetProperty();
  ~vtkPVScalarListWidgetProperty();
  
  float *Scalars;
  int NumberOfScalars;
  char **VTKCommands;
  int *NumberOfScalarsPerCommand;
  int NumberOfCommands;
  
private:
  vtkPVScalarListWidgetProperty(const vtkPVScalarListWidgetProperty&); // Not implemented
  void operator=(const vtkPVScalarListWidgetProperty&); // Not implemented
};

#endif
