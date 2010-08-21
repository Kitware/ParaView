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
// .NAME vtkSMSessionCore
// .SECTION Description
// vtkSMSessionCore is used by vtkSMSession. It encapsulates the
// vtkClientServerInterpreter as well as the vtkPMObject map.

#ifndef __vtkSMSessionCore_h
#define __vtkSMSessionCore_h

#include "vtkObject.h"
#include "vtkSMMessage.h"

class vtkClientServerInterpreter;
class vtkMultiProcessController;
class vtkSMProxyDefinitionManager;

class VTK_EXPORT vtkSMSessionCore : public vtkObject
{
public:
  static vtkSMSessionCore* New();
  vtkTypeMacro(vtkSMSessionCore, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Provides access to the interpreter.
  vtkGetObjectMacro(Interpreter, vtkClientServerInterpreter);

  // Description:
  // Provides access to the proxy definition manager.
  vtkGetObjectMacro(ProxyDefinitionManager, vtkSMProxyDefinitionManager);

  // Description:
  // Push the state message.
  virtual void PushState(vtkSMMessage* msg);

  // Description:
  // Pull the state message.
  virtual void PullState(vtkSMMessage* msg);

  // Description:
  // Invoke a method remotely
  virtual void Invoke(vtkSMMessage* msg);

  // Description:
  // Invoke a method remotely
  virtual void DeletePMObject(vtkSMMessage* msg);

//BTX
  enum MessageTypes
    {
    PUSH_STATE   = 12,
    PULL_STATE   = 13,
    INVOKE_STATE = 14
    };
  void PushStateSatelliteCallback();

protected:
  vtkSMSessionCore();
  ~vtkSMSessionCore();

  enum
    {
    ROOT_SATELLITE_RMI_TAG =  887822
    };

  vtkSMProxyDefinitionManager* ProxyDefinitionManager;
  vtkMultiProcessController* ParallelController;
  vtkClientServerInterpreter* Interpreter;
private:
  vtkSMSessionCore(const vtkSMSessionCore&); // Not implemented
  void operator=(const vtkSMSessionCore&);   // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
