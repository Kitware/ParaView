/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyManager - singleton responsible for creating proxies
// .SECTION Description
// vtkSMProxyManager is a singleton that creates and manages proxies.
// It maintains a map of XML elements (populated by the XML parser) from
// which it can create and initialize proxies and properties.
// Once a proxy is created, it can either be managed by the user code or
// the proxy manager. For latter, pass the control of the proxy to the
// manager with ManageProxy() and unregister it. At destruction, proxy
// manager deletes all managed proxies.
// .SECTION See Also
// vtkSMXMLParser

#ifndef __vtkSMProxyManager_h
#define __vtkSMProxyManager_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMProxy;
//BTX
struct vtkSMProxyManagerInternals;
//ETX

class VTK_EXPORT vtkSMProxyManager : public vtkSMObject
{
public:
  static vtkSMProxyManager* New();
  vtkTypeRevisionMacro(vtkSMProxyManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a group and proxy name, create and return a proxy instance.
  // The user has to delete the proxy when done.
  // NOTE: If this method is called from a scripting language, it may
  // not be possible to delete the returned object with Delete. 
  // The VTK wrappers handle New and Delete specially and may not allow
  // the deletion of object created through other methods. Use
  // UnRegister instead.
  vtkSMProxy* NewProxy(const char* groupName, const char* proxyName);

  // Description:
  // Used to pass the control of the proxy to the manager. The user code can
  // then release its reference count and not care about what happens
  // to the proxy. Managed proxies are deleted at destruction. NOTE:
  // The name has to be unique. If not, the existing proxy will be
  // replaced (and unregistered).
  void RegisterProxy(const char* name, vtkSMProxy* proxy);

  // Description:
  // Given its name returns a proxy. If not a managed proxy, returns 0.
  vtkSMProxy* GetProxy(const char* name);

  // Description:
  // Given its name, unregisters a proxy and remove it from the list
  // of managed proxies. 
  void UnRegisterProxy(const char* name);

  // Description:
  // Unregisters all managed proxies.
  void UnRegisterProxies();

  // Description:
  // Calls UpdateVTKObjects() on all managed proxies.
  void UpdateRegisteredProxies();

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();

  // Description:
  // Called by the XML parser to add an element from which a proxy
  // can be created. Called during parsing.
  void AddElement(
    const char* groupName, const char* name, vtkPVXMLElement* element);

  // Description:
  // Creates a new proxy and initializes it by calling ReadXMLAttributes()
  // with the right XML element.
  vtkSMProperty* NewProperty(vtkPVXMLElement* pelement);

  // Description:
  // Given an XML element create a proxy and all of it's properties.
  vtkSMProxy* NewProxy(vtkPVXMLElement* element);

//BTX
  friend class vtkSMXMLParser;
//ETX

private:
  vtkSMProxyManagerInternals* Internals;

private:
  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
};

#endif
