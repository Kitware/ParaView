/*=========================================================================

  Program:   ParaView
  Module:    vtkSITimeLabelProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSITimeLabelProperty
// .SECTION Description
// SIProperty that deals with TimeLabel annotation on Algorithm object type

#ifndef __vtkSITimeLabelProperty_h
#define __vtkSITimeLabelProperty_h

#include "vtkPVServerImplementationCoreModule.h" //needed for exports
#include "vtkSIProperty.h"

class VTKPVSERVERIMPLEMENTATIONCORE_EXPORT vtkSITimeLabelProperty : public vtkSIProperty
{
public:
  static vtkSITimeLabelProperty* New();
  vtkTypeMacro(vtkSITimeLabelProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSITimeLabelProperty();
  ~vtkSITimeLabelProperty();

  friend class vtkSIProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSITimeLabelProperty(const vtkSITimeLabelProperty&); // Not implemented
  void operator=(const vtkSITimeLabelProperty&); // Not implemented
//ETX
};

#endif
