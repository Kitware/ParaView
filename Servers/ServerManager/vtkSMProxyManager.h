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
// .NAME vtkSMProxyManager - singleton responsible for creating and managing proxies
// .SECTION Description
// vtkSMProxyManager is a singleton that creates and manages proxies.
// It maintains a map of XML elements (populated by the XML parser) from
// which it can create and initialize proxies and properties.
// Once a proxy is created, it can either be managed by the user code or
// the proxy manager. For latter, pass the control of the proxy to the
// manager with RegisterProxy() and unregister it. At destruction, proxy
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
  // The name has to be unique (per group). If not, the existing proxy will be
  // replaced (and unregistered). The proxy instances are grouped in collections
  // (not necessarily the same as the group in the XML configuration file).
  // These collections can be used to separate proxies based on their
  // functionality. For example, implicit planes can be grouped together
  // and the acceptable values of a proxy property can be restricted
  // (using a domain) to this collection.
  void RegisterProxy(const char* groupname, const char* name, vtkSMProxy* proxy);

  // Description:
  // Given its name (and group) returns a proxy. If not a managed proxy, 
  // returns 0.
  vtkSMProxy* GetProxy(const char* groupname, const char* name);
  vtkSMProxy* GetProxy(const char* name);

  // Description:
  // Returns the number of proxies in a group.
  unsigned int GetNumberOfProxies(const char* groupname);

  // Description:
  // Given a group and an index, returns the name of a proxy.
  // NOTE: This operation is slow.
  const char* GetProxyName(const char* groupname, unsigned int idx);

  // Description:
  // Given a group and a proxy, return it's name.
  // NOTE: This operation is slow.
  const char* GetProxyName(const char* groupname, vtkSMProxy* proxy);
  
  // Description:
  // Is the proxy is in the given group, return it's name, otherwise
  // return null. NOTE: Any following call to proxy manager might make
  // the returned pointer invalid.
  const char* IsProxyInGroup(vtkSMProxy* proxy, const char* groupname);

  // Description:
  // Given its name, unregisters a proxy and remove it from the list
  // of managed proxies. 
  void UnRegisterProxy(const char* groupname, const char* name);
  void UnRegisterProxy(const char* name);

  // Description:
  // Unregisters all managed proxies.
  void UnRegisterProxies();

  // Description:
  // Calls UpdateVTKObjects() on all managed proxies.
  void UpdateRegisteredProxies(const char* groupname);
  void UpdateRegisteredProxies();

  // Description:
  // Save the state of the server manager in XML format in a file.
  // This saves the state of all proxies and properties. NOTE: The XML
  // format is still evolving.
  void SaveState(const char* filename);

  // Description:
  // Saves the state of the object in XML format. Should
  // be overwritten by proxies and properties.
  virtual void SaveState(const char*, ostream*, vtkIndent);

  // Description:
  // Given a group name, create prototypes and store them
  // in a instance group called groupName_prototypes.
  void InstantiateGroupPrototypes(const char* groupName);

  // Description:
  // Returns the number of XML groups from which proxies can
  // be created.
  unsigned int GetNumberOfXMLGroups();

  // Description:
  // Returns the name of nth XML group.
  const char* GetXMLGroupName(unsigned int n);

  // Description:
  // Returns 1 if a proxy element of given group and exists, 0
  // otherwise. If a proxy element does not exist, a call to
  // NewProxy() will fail.
  int ProxyElementExists(const char* groupName,  const char* proxyName);

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();

  // Description:
  // Called by the XML parser to add an element from which a proxy
  // can be created. Called during parsing.
  void AddElement(
    const char* groupName, const char* name, vtkPVXMLElement* element);

//BTX
  friend class vtkSMXMLParser;
  friend class vtkSMProxyIterator;
  friend class vtkSMProxy;
//ETX

  // Description:
  // Given an XML element and group name create a proxy 
  // and all of it's properties.
  vtkSMProxy* NewProxy(vtkPVXMLElement* element, const char* groupname);

  // Description:
  // Given the proxy name and group name, returns the XML element for
  // the proxy.
  vtkPVXMLElement* GetProxyElement(const char* groupName,
    const char* proxyName);

private:
  vtkSMProxyManagerInternals* Internals;

private:
  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
};

#endif
