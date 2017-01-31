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
/**
 * @class   vtkSMMultiServerSourceProxy
 *       - proxy use to fetch data from distributed servers
 *
 * vtkSMMultiServerSourceProxy can be usefull in case of multi-server setup
 * when the user want to display a data object that belong to another server
 * into its local built-in view.
*/

#ifndef vtkSMMultiServerSourceProxy_h
#define vtkSMMultiServerSourceProxy_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMSourceProxy.h"

class vtkSMProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMMultiServerSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMMultiServerSourceProxy* New();
  vtkTypeMacro(vtkSMMultiServerSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Bind proxy with a given external proxy
   */
  virtual void SetExternalProxy(vtkSMSourceProxy* proxyFromAnotherServer, int port = 0);

  /**
   * Return the proxy that is currently binded if any otherwise return NULL;
   */
  virtual vtkSMSourceProxy* GetExternalProxy();

  /**
   * Marks the selection proxies dirty as well as chain to superclass.
   */
  virtual void MarkDirty(vtkSMProxy* modifiedProxy) VTK_OVERRIDE;

  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  virtual void LoadState(const vtkSMMessage* message, vtkSMProxyLocator* locator) VTK_OVERRIDE;

protected:
  vtkSMMultiServerSourceProxy();
  ~vtkSMMultiServerSourceProxy();

  virtual void UpdateState();

  vtkIdType RemoteProxySessionID;
  vtkIdType RemoteProxyID;
  int PortToExport;

private:
  vtkSMMultiServerSourceProxy(const vtkSMMultiServerSourceProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMMultiServerSourceProxy&) VTK_DELETE_FUNCTION;
};

#endif
