/*=========================================================================

  Program:   ParaView
  Module:    vtkSMObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMObject - superclass for most server manager classes
// .SECTION Description
// vtkSMObject is mostly to tag a class hierarchy that it belong to the
// servermanager.

#ifndef __vtkSMObject_h
#define __vtkSMObject_h

#include "vtkObject.h"

class vtkSMApplication;

class VTK_EXPORT vtkSMObject : public vtkObject
{
public:
  static vtkSMObject* New();
  vtkTypeMacro(vtkSMObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMObject();
  ~vtkSMObject();

private:
  vtkSMObject(const vtkSMObject&); // Not implemented
  void operator=(const vtkSMObject&); // Not implemented
};

#endif
