/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeRangeProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSITimeRangeProperty
// .SECTION Description
// SIProperty that deals with TimeRange on Algorithm object type

#ifndef __vtkSITimeRangeProperty_h
#define __vtkSITimeRangeProperty_h

#include "vtkSIProperty.h"

class VTK_EXPORT vtkSITimeRangeProperty : public vtkSIProperty
{
public:
  static vtkSITimeRangeProperty* New();
  vtkTypeMacro(vtkSITimeRangeProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSITimeRangeProperty();
  ~vtkSITimeRangeProperty();

  friend class vtkSIProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSITimeRangeProperty(const vtkSITimeRangeProperty&); // Not implemented
  void operator=(const vtkSITimeRangeProperty&); // Not implemented
//ETX
};

#endif
