/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArrayProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIDataArrayProperty - InformationOnly property
// .SECTION Description
// SIProperty that deals with vtkDataArray object type

#ifndef __vtkSIDataArrayProperty_h
#define __vtkSIDataArrayProperty_h

#include "vtkSIProperty.h"

class VTK_EXPORT vtkSIDataArrayProperty : public vtkSIProperty
{
public:
  static vtkSIDataArrayProperty* New();
  vtkTypeMacro(vtkSIDataArrayProperty, vtkSIProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIDataArrayProperty();
  ~vtkSIDataArrayProperty();

  friend class vtkSIProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkSIDataArrayProperty(const vtkSIDataArrayProperty&); // Not implemented
  void operator=(const vtkSIDataArrayProperty&); // Not implemented
//ETX
};

#endif
