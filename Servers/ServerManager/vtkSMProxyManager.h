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
// Every proxy has a ConnectionID associated with it which indicates the
// Server connection on which the proxy exists. Changing the ConnectionID
// must be done immediately after the proxy is instantiated.
// .SECTION See Also
// vtkSMXMLParser

#ifndef __vtkSMProxyManager_h
#define __vtkSMProxyManager_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMCompoundProxy;
class vtkSMDocumentation;
class vtkSMLink;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyManagerObserver;
class vtkSMStateLoader;
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
  // Returns a vtkSMDocumentation object with the documentation
  // for the proxy with given name and group name. Note that the name and group
  // name are not those with the which the proxy is registered, but those
  // with which the proxy is created i.e. the arguments used for NewProxy().
  vtkSMDocumentation* GetProxyDocumentation(const char* groupName, 
    const char* proxyName);

  // Description:
  // Returns a vtkSMDocumentation object with the documentation
  // for the given property of the proxy with given name and group name. 
  // Note that the name and group
  // name are not those with the which the proxy is registered, but those
  // with which the proxy is created i.e. the arguments used for NewProxy().
  // Also, the property name is the name of an exposed property.
  vtkSMDocumentation* GetPropertyDocumentation(const char* groupName, 
    const char* proxyName, const char* propertyName);

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
  // Returns the prototype proxy for the given type. This method may create
  // a new prototype proxy, is one does not already exist.
  vtkSMProxy* GetPrototypeProxy(const char* groupname, const char* name);

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
  // Given a proxy, unregisters it. This method unregisters the proxy
  // from all the groups it has been registered in.
  void UnRegisterProxy(vtkSMProxy* proxy);

  // Description:
  // Unregisters all managed proxies.
  void UnRegisterProxies();

  // Description:
  // Calls UpdateVTKObjects() on all managed proxies.
  // If modified_only flag is set, then UpdateVTKObjects will be called 
  // only those proxies that have any properties that were modifed i.e.
  // not pushed to the VTK objects.
  void UpdateRegisteredProxies(const char* groupname, int modified_only = 1);
  void UpdateRegisteredProxies(int modified_only=1);

  // Description:
  // Register proxy/property links with the server manager. The linknames
  // must be unique, if a link with the given name already exists, it will be replaced.
  void RegisterLink(const char* linkname, vtkSMLink* link);

  // Description:
  // Unregister a proxy or property link previously registered with the given name.
  void UnRegisterLink(const char* linkname);

  // Description:
  // Get the link registered with the given name. If no such link exists,
  // returns NULL.
  vtkSMLink* GetRegisteredLink(const char* linkname);
 
  // Description:
  // Unregister all registered proxy/property links.
  void UnRegisterAllLinks();
  
  // Description:
  // Register a compound proxy definition with the proxy manager. This
  // definition (represented as a tree of vtkPVXMLElement objects) can
  // then be used to instantiate a copy of the compound proxy. See
  // vtkSMCompoundProxy.h for details.
  void RegisterCompoundProxyDefinition(const char* name, vtkPVXMLElement* top);

  // Description:
  // Given its name, unregisters a compound proxy definition.
  void UnRegisterCompoundProxyDefinition(const char* name);

  // Description:
  // Unregisters all registered compound proxy definitions.
  void UnRegisterCompoundProxyDefinitions();

  // Description:
  // Returns a registered compound proxy definition.
  vtkPVXMLElement* GetCompoundProxyDefinition(const char* name);

  // Description:
  // Creates a compound proxy from compound proxy definition.
  vtkSMCompoundProxy* NewCompoundProxy(const char* name); 

  // Description:
  // Load compound proxy definitions and register them.
  void LoadCompoundProxyDefinitions(const char* filename);
  void LoadCompoundProxyDefinitions(vtkPVXMLElement* root);

  // Description:
  // Save registered compound proxy definitions.
  void SaveCompoundProxyDefinitions(const char* filename);
  void SaveCompoundProxyDefinitions(vtkPVXMLElement* root);

  // Description:
  // Loads the state of the server manager from XML.
  // If loader is not specified, a vtkSMStateLoader instance is used.
  void LoadState(const char* filename, vtkSMStateLoader* loader=NULL);
  void LoadState(vtkPVXMLElement* rootElement, vtkSMStateLoader* loader=NULL);
  
  // Description:
  // Load the state for a particular connection.
  // If loader is not specified, a vtkSMStateLoader instance is used.
  void LoadState(vtkPVXMLElement* rootElement, vtkIdType id, vtkSMStateLoader* loader=NULL);
  void LoadState(const char* filename, vtkIdType id, vtkSMStateLoader* loader=NULL);
  
  // Description:
  // Save the state of the server manager in XML format in a file.
  // This saves the state of all proxies and properties. NOTE: The XML
  // format is still evolving.
  void SaveState(const char* filename);
  void SaveState(vtkPVXMLElement* rootElement);

  // Description:
  // Given a group name, create prototypes and store them
  // in a instance group called groupName_prototypes.
  // Prototypes have their ConnectionID set to the SelfConnection.
  void InstantiateGroupPrototypes(const char* groupName);

  // Description:
  // Creates protytpes for all known proxy types.
  void InstantiatePrototypes();

  // Description:
  // Returns the number of XML groups from which proxies can
  // be created.
  unsigned int GetNumberOfXMLGroups();

  // Description:
  // Returns the name of nth XML group.
  const char* GetXMLGroupName(unsigned int n);

  // Description:
  // Returns the number of proxies under the group with \c groupName for which
  // proxies can be created.
  unsigned int GetNumberOfXMLProxies(const char* groupName);

  // Description:
  // Returns the name for the nth XML proxy element under the
  // group with name \c groupName.
  const char* GetXMLProxyName(const char* groupName, unsigned int n);

  // Description:
  // Returns 1 if a proxy element of given group and exists, 0
  // otherwise. If a proxy element does not exist, a call to
  // NewProxy() will fail.
  int ProxyElementExists(const char* groupName,  const char* proxyName);

//BTX
  struct RegisteredProxyInformation
  {
    vtkSMProxy* Proxy;
    const char* GroupName;
    const char* ProxyName;
    // Set when the register/unregister event if fired for registration of 
    // a compound proxy definition.
    int IsCompoundProxyDefinition;
  };

  struct ModifiedPropertyInformation
    {
    vtkSMProxy* Proxy;
    const char* PropertyName;
    };
//ETX

  // Description:
  // Get if there are any registered proxies that have their properties in
  // a modified state.
  int AreProxiesModified();

  // Description:
  // The server manager configuration XML may define <Hints /> element for a 
  // proxy. Hints are metadata associated with the proxy. The Server Manager
  // does not (and should not) interpret the hints. Hints provide a mechanism
  // to add GUI pertinant information to the server manager XML.
  // Returns the XML element for the hints associated with this proxy,
  // if any, otherwise returns NULL. 
  vtkPVXMLElement* GetHints(const char* xmlgroup, const char* xmlname);
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
  friend class vtkSMProxyDefinitionIterator;
  friend class vtkSMProxy;
  friend class vtkSMProxyManagerObserver;
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

  // Description:
  // Handles events.
  virtual void ExecuteEvent(vtkObject* obj, unsigned long event, void* data);

  // Description:
  // Mark/UnMark a proxy as modified.
  void MarkProxyAsModified(vtkSMProxy*);
  void UnMarkProxyAsModified(vtkSMProxy*);

  // Description:
  // Save/Load registered link states.
  void SaveRegisteredLinks(vtkPVXMLElement* root);

private:
  vtkSMProxyManagerInternals* Internals;
  vtkSMProxyManagerObserver* Observer;

private:
  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
};

#endif
