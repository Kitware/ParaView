/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScalarListWidgetProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVScalarListWidgetProperty
// .SECTION Description

#ifndef __vtkPVScalarListWidgetProperty_h
#define __vtkPVScalarListWidgetProperty_h

#include "vtkPVWidgetProperty.h"

class VTK_EXPORT vtkPVScalarListWidgetProperty : public vtkPVWidgetProperty
{
public:
  static vtkPVScalarListWidgetProperty* New();
  vtkTypeRevisionMacro(vtkPVScalarListWidgetProperty, vtkPVWidgetProperty);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void AcceptInternal();
  
//BTX
  void SetVTKCommands(int numCmds, const char * const*cmds, int *numScalars);
//ETX
  void SetScalars(int num, float *scalars);
  void AddScalar(float scalar);
  float* GetScalars() { return this->Scalars; }
  float GetScalar(int idx);
  vtkGetMacro(NumberOfScalars, int);

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
