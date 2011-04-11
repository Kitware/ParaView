/*=========================================================================

  Program:   ParaView
  Module:    vtkSIContextArraysProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIContextArraysProperty - provides information about array for a
// chart.
// .SECTION Description
// This class iterates through a vtkTable and fills the supplied property with
// column names matching their current column index.

#ifndef __vtkSIContextArraysProperty_h
#define __vtkSIContextArraysProperty_h

#include "vtkSIProperty.h"

class VTK_EXPORT vtkSIContextArraysProperty : public vtkSIProperty
{
public:
  static vtkSIContextArraysProperty* New();
  vtkTypeMacro(vtkSIContextArraysProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIContextArraysProperty();
  ~vtkSIContextArraysProperty();

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSIContextArraysProperty(const vtkSIContextArraysProperty&); // Not implemented
  void operator=(const vtkSIContextArraysProperty&); // Not implemented
//ETX
};

#endif
