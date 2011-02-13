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
// .NAME vtkPMContextArraysProperty - provides information about array for a
// chart.
// .SECTION Description
// This class iterates through a vtkTable and fills the supplied property with
// column names matching their current column index.

#ifndef __vtkPMContextArraysProperty_h
#define __vtkPMContextArraysProperty_h

#include "vtkPMProperty.h"

class VTK_EXPORT vtkPMContextArraysProperty : public vtkPMProperty
{
public:
  static vtkPMContextArraysProperty* New();
  vtkTypeMacro(vtkPMContextArraysProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMContextArraysProperty();
  ~vtkPMContextArraysProperty();

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkPMContextArraysProperty(const vtkPMContextArraysProperty&); // Not implemented
  void operator=(const vtkPMContextArraysProperty&); // Not implemented
//ETX
};

#endif
