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
/**
 * @class   vtkSMObject
 * @brief   superclass for most server manager classes
 *
 * vtkSMObject is mostly to tag a class hierarchy that it belong to the
 * servermanager.
*/

#ifndef vtkSMObject_h
#define vtkSMObject_h

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports

class vtkSMApplication;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMObject : public vtkObject
{
public:
  static vtkSMObject* New();
  vtkTypeMacro(vtkSMObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkSMObject();
  ~vtkSMObject();

private:
  vtkSMObject(const vtkSMObject&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMObject&) VTK_DELETE_FUNCTION;
};

#endif
