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
// .NAME vtkSMVectorProperty -
// .SECTION Description

#ifndef __vtkSMVectorProperty_h
#define __vtkSMVectorProperty_h

#include "vtkSMProperty.h"

class VTK_EXPORT vtkSMVectorProperty : public vtkSMProperty
{
public:
  vtkTypeRevisionMacro(vtkSMVectorProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  virtual int GetNumberOfElements() = 0;

  // Description:
  virtual void SetNumberOfElements(int num) = 0;

  // Description:
  vtkGetMacro(RepeatCommand, int);
  vtkSetMacro(RepeatCommand, int);
  vtkBooleanMacro(RepeatCommand, int);

  // Description:
  vtkGetMacro(NumberOfElementsPerCommand, int);
  vtkSetMacro(NumberOfElementsPerCommand, int);

  // Description:
  vtkGetMacro(UseIndex, int);
  vtkSetMacro(UseIndex, int);

protected:
  vtkSMVectorProperty();
  ~vtkSMVectorProperty();

  int RepeatCommand;
  int NumberOfElementsPerCommand;
  int UseIndex;

  virtual int ReadXMLAttributes(vtkPVXMLElement* element);

private:
  vtkSMVectorProperty(const vtkSMVectorProperty&); // Not implemented
  void operator=(const vtkSMVectorProperty&); // Not implemented
};

#endif
