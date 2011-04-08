/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPythonTraceObserver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPythonTraceObserver - A helper class used by the smtrace python module
// .SECTION Description
// This class is instantiated by the smtrace python module.  It listens for invoked
// events from the proxy manager and relays them to python.
//

#ifndef __vtkSMPythonTraceObserver_h
#define __vtkSMPythonTraceObserver_h

#include "vtkSMObject.h"

class vtkSMPythonTraceObserverCommandHelper;
class vtkSMProxy;

class VTK_EXPORT vtkSMPythonTraceObserver : public vtkSMObject
{
public:
  static vtkSMPythonTraceObserver* New();
  vtkTypeMacro(vtkSMPythonTraceObserver, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSMProxy* GetLastPropertyModifiedProxy();
  vtkSMProxy* GetLastProxyRegistered();
  vtkSMProxy* GetLastProxyUnRegistered();
  const char* GetLastPropertyModifiedName();
  const char* GetLastProxyRegisteredGroup();
  const char* GetLastProxyRegisteredName();
  const char* GetLastProxyUnRegisteredGroup();
  const char* GetLastProxyUnRegisteredName();

//BTX
protected:
  vtkSMPythonTraceObserver();
  ~vtkSMPythonTraceObserver();

  friend class vtkSMPythonTraceObserverCommandHelper;

  // Description:
  // Event handler
  virtual void ExecuteEvent(vtkObject* called, unsigned long eventid, void* data);

private:

  class vtkInternal;
  vtkInternal* Internal;

  vtkSMPythonTraceObserverCommandHelper* Observer;

  vtkSMPythonTraceObserver(const vtkSMPythonTraceObserver&); // Not implemented.
  void operator=(const vtkSMPythonTraceObserver&); // Not implemented.

//ETX
};

#endif
