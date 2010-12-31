/*=========================================================================

  Program:   ParaView
  Module:    vtkPMTimeRangeProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMTimeRangeProperty
// .SECTION Description
// PMProperty that deals with TimeRange on Algorithm object type

#ifndef __vtkPMTimeRangeProperty_h
#define __vtkPMTimeRangeProperty_h

#include "vtkPMProperty.h"

class VTK_EXPORT vtkPMTimeRangeProperty : public vtkPMProperty
{
public:
  static vtkPMTimeRangeProperty* New();
  vtkTypeMacro(vtkPMTimeRangeProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMTimeRangeProperty();
  ~vtkPMTimeRangeProperty();

  friend class vtkPMProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkPMTimeRangeProperty(const vtkPMTimeRangeProperty&); // Not implemented
  void operator=(const vtkPMTimeRangeProperty&); // Not implemented
//ETX
};

#endif
