/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiServerSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiServerSourceProxy
//       - proxy use to fetch data from distributed servers
// .SECTION Description
// vtkSMMultiServerSourceProxy can be usefull in case of multi-server setup
// when the user want to display a data object that belong to another server
// into its local built-in view.

#ifndef __vtkSMMultiServerSourceProxy_h
#define __vtkSMMultiServerSourceProxy_h

#include "vtkSMSourceProxy.h"

class vtkSMProxy;

class VTK_EXPORT vtkSMMultiServerSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMMultiServerSourceProxy* New();
  vtkTypeMacro(vtkSMMultiServerSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bind proxy with a given external proxy
  virtual void SetExternalProxy(vtkSMSourceProxy* proxyFromAnotherServer, int port = 0);

  // Description:
  // Return the proxy that is currently binded if any otherwise return NULL;
  virtual vtkSMSourceProxy* GetExternalProxy();

//BTX

  // Description:
  // Marks the selection proxies dirty as well as chain to superclass.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* message, vtkSMProxyLocator* locator);

protected:
  vtkSMMultiServerSourceProxy();
  ~vtkSMMultiServerSourceProxy();

  virtual void UpdateState();

  vtkIdType RemoteProxySessionID;
  vtkIdType RemoteProxyID;
  int PortToExport;

private:
  vtkSMMultiServerSourceProxy(const vtkSMMultiServerSourceProxy&); // Not implemented
  void operator=(const vtkSMMultiServerSourceProxy&); // Not implemented
//ETX
};

#endif
