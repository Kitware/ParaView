/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSessionCoreInterpreterHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSessionCoreInterpreterHelper
// .SECTION Description
// vtkPVSessionCoreInterpreterHelper is used by vtkPVSessionCore to avoid a
// circular reference between the vtkPVSessionCore instance and its Interpreter.

#ifndef __vtkPVSessionCoreInterpreterHelper_h
#define __vtkPVSessionCoreInterpreterHelper_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkObject;
class vtkSIObject;
class vtkPVProgressHandler;
class vtkProcessModule;
class vtkPVSessionCore;
class vtkMPIMToNSocketConnection;

class VTK_EXPORT vtkPVSessionCoreInterpreterHelper : public vtkObject
{
public:
  static vtkPVSessionCoreInterpreterHelper* New();
  vtkTypeMacro(vtkPVSessionCoreInterpreterHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSIObject* GetSIObject(vtkTypeUInt32 gid);
  vtkObjectBase* GetVTKObject(vtkTypeUInt32 gid);

  vtkProcessModule* GetProcessModule();
  vtkPVProgressHandler* GetActiveProgressHandler();

  // Description:
  // Sets and initializes the MPIMToNSocketConnection for communicating between
  // data-server and render-server.
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection*);

  void SetCore(vtkPVSessionCore*);

//BTX
protected:
  vtkPVSessionCoreInterpreterHelper();
  ~vtkPVSessionCoreInterpreterHelper();

  vtkWeakPointer<vtkPVSessionCore> Core;
private:
  vtkPVSessionCoreInterpreterHelper(const vtkPVSessionCoreInterpreterHelper&); // Not implemented
  void operator=(const vtkPVSessionCoreInterpreterHelper&); // Not implemented
//ETX
};

#endif
