/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVServerObject - Base class for server-side helper objects.
// .SECTION Description

#ifndef __vtkPVServerObject_h
#define __vtkPVServerObject_h

#include "vtkObject.h"

class vtkProcessModule;

class VTK_EXPORT vtkPVServerObject : public vtkObject
{
public:
  static vtkPVServerObject* New();
  vtkTypeMacro(vtkPVServerObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the process module for this node.
  virtual void SetProcessModule(vtkProcessModule*);
  vtkGetObjectMacro(ProcessModule, vtkProcessModule);

protected:
  vtkPVServerObject();
  ~vtkPVServerObject();

  // The process module for this node.
  vtkProcessModule* ProcessModule;

private:
  vtkPVServerObject(const vtkPVServerObject&); // Not implemented
  void operator=(const vtkPVServerObject&); // Not implemented
};

#endif
