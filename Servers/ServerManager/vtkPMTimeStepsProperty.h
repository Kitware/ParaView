/*=========================================================================

  Program:   ParaView
  Module:    vtkPMTimeStepsProperty.h

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

#ifndef __vtkPMTimeStepsProperty_h
#define __vtkPMTimeStepsProperty_h

#include "vtkPMProperty.h"
#include "vtkSMMessage.h"

class VTK_EXPORT vtkPMTimeStepsProperty : public vtkPMProperty
{
public:
  static vtkPMTimeStepsProperty* New();
  vtkTypeMacro(vtkPMTimeStepsProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMTimeStepsProperty();
  ~vtkPMTimeStepsProperty();

  friend class vtkPMProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkPMTimeStepsProperty(const vtkPMTimeStepsProperty&); // Not implemented
  void operator=(const vtkPMTimeStepsProperty&); // Not implemented
//ETX
};

#endif
