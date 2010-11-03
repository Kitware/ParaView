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
// .NAME vtkPMDataArrayProperty
// .SECTION Description
// PMProperty that deals with vtkDataArray object type

#ifndef __vtkPMDataArrayProperty_h
#define __vtkPMDataArrayProperty_h

#include "vtkPMProperty.h"
#include "vtkSMMessage.h"

class VTK_EXPORT vtkPMDataArrayProperty : public vtkPMProperty
{
public:
  static vtkPMDataArrayProperty* New();
  vtkTypeMacro(vtkPMDataArrayProperty, vtkPMProperty);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMDataArrayProperty();
  ~vtkPMDataArrayProperty();

  friend class vtkPMProxy;

  // Description:
  // Pull the current state of the underneath implementation
  virtual bool Pull(vtkSMMessage*);

private:
  vtkPMDataArrayProperty(const vtkPMDataArrayProperty&); // Not implemented
  void operator=(const vtkPMDataArrayProperty&); // Not implemented
//ETX
};

#endif
