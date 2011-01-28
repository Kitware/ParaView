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
// .NAME vtkSMSessionCoreInterpreterHelper
// .SECTION Description
// vtkSMSessionCoreInterpreterHelper is used by vtkSMSessionCore to avoid a
// circular reference between the vtkSMSessionCore instance and its Interpreter.

#ifndef __vtkSMSessionCoreInterpreterHelper_h
#define __vtkSMSessionCoreInterpreterHelper_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMSessionCore;
class vtkPMObject;
class vtkObject;

class VTK_EXPORT vtkSMSessionCoreInterpreterHelper : public vtkObject
{
public:
  static vtkSMSessionCoreInterpreterHelper* New();
  vtkTypeMacro(vtkSMSessionCoreInterpreterHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkPMObject* GetPMObject(vtkTypeUInt32 gid);
  vtkObjectBase* GetVTKObject(vtkTypeUInt32 gid);

  void SetCore(vtkSMSessionCore*);

//BTX
protected:
  vtkSMSessionCoreInterpreterHelper();
  ~vtkSMSessionCoreInterpreterHelper();

  vtkWeakPointer<vtkSMSessionCore> Core;
private:
  vtkSMSessionCoreInterpreterHelper(const vtkSMSessionCoreInterpreterHelper&); // Not implemented
  void operator=(const vtkSMSessionCoreInterpreterHelper&); // Not implemented
//ETX
};

#endif
