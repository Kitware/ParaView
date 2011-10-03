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
//
// vtkSMProxyManager is designed to work with one active session at a time. When
// a session closes, it cleans up the proxy manager that releases known
// definitions as well as registered proxies. When a new session is established
// it updates the definitions and is the ready to create new proxies.
// .SECTION See Also
// vtkSMProxyDefinitionManager
//
// Basic XML Proxy definition documentation:
//
// ------------- Proxy definition -------------
//  <SourceProxy                   => Will create vtkSM + SourceProxy class
//         name="SphereSource"     => Key used to create the proxy
//         class="vtkSphereSource" => Concreate vtkClass that do the real job
//         label="Sphere">         => Nice name used in menu and python shell
//
// ----------- Property definition -----------
//    <DoubleVectorProperty        => Will create vtkSM + DoubleVectorProperty
//                                    and vtkSI + DoubleVectorProperty class by
//                                    default.
//         name="Center"           => Name of the property:
//                                    vtkPropertyHelper(proxy, "Center").Set(0,1,2)
//         command="SetCenter"     => Real method name that will be called on
//                                    vtkObject when the property will be updated.
//         number_of_elements="3"  => Size of the vector
//         animateable="1"         => Tell the animation view that property
//                                    can be used as an evolving property
//         default_values="0 0 0"> => The value that will be set at the
//    </DoubleVectorProperty>         construction to the VTK object
//  </SourceProxy>
//
// For custom behaviour the user can add some extra attributes:
//
//  - We can specify a custom SIProperty class to handle in a custom way the
//    data on the server.
//      <StringVectorProperty          => vtkSMStringVectorProperty class
//         name="ElementBlocksInfo"    => Property name
//         information_only="1"        => Can only be used to fecth data
//         si_class="vtkSISILProperty" => Class name to instanciate on the other side
//         subtree="Blocks"/>          => Extra attribute used by vtkSISILProperty
//
//  - We can trigger after any update a command to be executed
//      <Proxy name="LookupTable"
//             class="vtkLookupTable"
//             post_push="Build"       => The method Build() will be called each
//                                        time a new property value is pushed to
//                                        the VTK object.
//             processes="dataserver|renderserver|client" >
//
//  - We can force any property to push its value as soon has it get changed
//          <Property name="ResetFieldCriteria"
//             command="ResetFieldCriteria"
//             immediate_update="1">     => Modifying the property will result
//                                          of a push of it and the execution
//                                          of the command on the vtkObject.
//
//  - To show a source proxy or a filter inside the menu of ParaView we use a hint
//       <SourceProxy ...>
//           <Hints>
//              <ShowInGUI                   => The category attribute allow to
//                  category="PersoFilter"/>    specify in which sub-menu this
//                                              proxy should be in. (optional)
//           </Hints>
//       </SourceProxy>

#ifndef __vtkSMProxyManager_h
#define __vtkSMProxyManager_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage.

class vtkCollection;
class vtkEventForwarderCommand;
class vtkPVXMLElement;
class vtkSMCompoundSourceProxy;
class vtkSMDocumentation;
class vtkSMGlobalPropertiesManager;
class vtkSMLink;
class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyDefinitionManager;
class vtkSMProxyIterator;
class vtkSMProxyLocator;
class vtkSMProxyManagerObserver;
class vtkSMProxyManagerProxySet;
class vtkSMProxySelectionModel;
class vtkSMReaderFactory;
class vtkSMSession;
class vtkSMStateLoader;
class vtkSMWriterFactory;
class vtkStringList;
class vtkSMPipelineState;
class vtkSMStateLocator;

//BTX
struct vtkSMProxyManagerInternals;
struct vtkClientServerID;
//ETX

class VTK_EXPORT vtkSMProxyManager : public vtkSMObject
{
public:
  // Description:
  // Return the GlobalID that should be used to refer to the ProxyManager state
  static vtkTypeUInt32 GetReservedGlobalID();

  static vtkSMProxyManager* New();
  vtkTypeMacro(vtkSMProxyManager, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the session on which this proxymanager is working.
  // Note that session is not reference counted.
  void SetSession(vtkSMSession*);
  vtkGetObjectMacro(Session, vtkSMSession);

  // Description:
  // Given a group and proxy name, create and return a proxy instance.
  // The user has to delete the proxy when done.
  // NOTE: If this method is called from a scripting language, it may
  // not be possible to delete the returned object with Delete.
  // The VTK wrappers handle New and Delete specially and may not allow
  // the deletion of object created through other methods. Use
  // UnRegister instead.
  vtkSMProxy* NewProxy(const char* groupName, const char* proxyName,
                       const char* subProxyName = NULL);

  // Description:
  // Returns a vtkSMDocumentation object with the documentation
  // for the proxy with given name and group name. Note that the name and group
  // name are not those with the which the proxy is registered, but those
  // with which the proxy is created i.e. the arguments used for NewProxy().
  vtkSMDocumentation* GetProxyDocumentation( const char* groupName,
                                             const char* proxyName );

  // Description:
  // Returns a vtkSMDocumentation object with the documentation
  // for the given property of the proxy with given name and group name.
  // Note that the name and group
  // name are not those with the which the proxy is registered, but those
  // with which the proxy is created i.e. the arguments used for NewProxy().
  // Also, the property name is the name of an exposed property.
  vtkSMDocumentation* GetPropertyDocumentation( const char* groupName,
                                                const char* proxyName,
                                                const char* propertyName );

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
  // Returns all proxies registered under the given group with the given name.
  // The collection is cleared before the proxies are added to it.
  void GetProxies(const char* groupname, const char* name,
    vtkCollection* collection);

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
  // Given a group and a proxy, return it's name. The proxy may be registered
  // as two different names under the same group, this only returns the  first hit
  // name, use GetProxyNames() to obtain the list of names for the proxy.
  // NOTE: This operation is slow.
  const char* GetProxyName(const char* groupname, vtkSMProxy* proxy);

  // Description:
  // Given a group and a proxy, return all its names. This clears the
  // \c names list before populating it with the names for the proxy under
  // the group.
  // NOTE: This operation is slow.
  void GetProxyNames(const char* groupname, vtkSMProxy* proxy,
    vtkStringList* names);

  // Description:
  // Is the proxy is in the given group, return it's name, otherwise
  // return null. NOTE: Any following call to proxy manager might make
  // the returned pointer invalid.
  const char* IsProxyInGroup(vtkSMProxy* proxy, const char* groupname);

  // Description:
  // Given its name, unregisters a proxy and remove it from the list
  // of managed proxies.
  void UnRegisterProxy(const char* groupname, const char* name, vtkSMProxy*);
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
  // Updates all registered proxies in order, respecting dependencies
  // among each other. This is used after loading state or after instantiating
  // a compound proxy. This uses the "UpdateInputProxies" flag which
  // vtkSMProxy checks in UpdateVTKObjects() to call UpdateVTKObjects() on the input
  // proxies as well if the flag is set.
  void UpdateRegisteredProxiesInOrder(int modified_only=1);
  void UpdateProxyInOrder(vtkSMProxy* proxy);

  // Description:
  // Get the number of registered links with the server manager.
  int GetNumberOfLinks();

  // Description:
  // Get the name of a link.
  const char* GetLinkName(int index);

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
  // Register a custom proxy definition with the proxy manager.
  // This can be a compound proxy definition (look at
  // vtkSMCompoundSourceProxy.h) or a regular proxy definition.
  // For all practical purposes, there's no difference between a proxy
  // definition added using this method or by parsing a server manager
  // configuration file.
  void RegisterCustomProxyDefinition(
    const char* group, const char* name, vtkPVXMLElement* top);

  // Description:
  // Given its name, unregisters a custom proxy definition.
  // Note that this can only be used to remove definitions added using
  // RegisterCustomProxyDefinition(), cannot be used to remove definitions
  // loaded using vtkSMXMLParser.
  void UnRegisterCustomProxyDefinition(const char* group, const char* name);

  // Description:
  // Unregisters all registered custom proxy definitions.
  // Note that this can only be used to remove definitions added using
  // RegisterCustomProxyDefinition(), cannot be used to remove definitions
  // loaded using vtkSMXMLParser.
  void UnRegisterCustomProxyDefinitions();

  // Description:
  // Returns a registered proxy definition.
  vtkPVXMLElement* GetProxyDefinition(const char* group, const char* name);

  // Description:
  // Load custom proxy definitions and register them.
  void LoadCustomProxyDefinitions(const char* filename);
  void LoadCustomProxyDefinitions(vtkPVXMLElement* root);

  // Description:
  // Save registered custom proxy definitions.
  void SaveCustomProxyDefinitions(vtkPVXMLElement* root);

  // Description:
  // Loads the state of the server manager from XML.
  // If loader is not specified, a vtkSMStateLoader instance is used.
  void LoadXMLState(const char* filename, vtkSMStateLoader* loader=NULL);
  void LoadXMLState(vtkPVXMLElement* rootElement, vtkSMStateLoader* loader=NULL);

  // Description:
  // Save the state of the server manager in XML format in a file.
  // This saves the state of all proxies and properties.
  void SaveXMLState(const char* filename);

  // Description:
  // Saves the state of the server manager as XML, and returns the
  // vtkPVXMLElement for the root of the state.
  // Note this this method allocates a new vtkPVXMLElement object,
  // it's the caller's responsibility to free it by calling Delete().
  vtkPVXMLElement* SaveXMLState();

  // Description:
  // Given a group name, create prototypes and store them
  // in a instance group called groupName_prototypes.
  // Prototypes have their ConnectionID set to the SelfConnection.
  void InstantiateGroupPrototypes(const char* groupName);

  // Description:
  // Creates protytpes for all known proxy types.
  void InstantiatePrototypes();

  // Description:
  // Return true if the XML Definition was found by vtkSMProxyDefinitionManager
  bool HasDefinition( const char* groupName, const char* proxyName );

//BTX
  struct RegisteredProxyInformation
  {
    vtkSMProxy* Proxy;
    const char* GroupName;
    const char* ProxyName;
    // Set when the register/unregister event if fired for registration of
    // a compound proxy definition.
    unsigned int Type;

    enum
      {
      PROXY =0x1,
      COMPOUND_PROXY_DEFINITION = 0x2,
      LINK = 0x3,
      GLOBAL_PROPERTIES_MANAGER = 0x4
      };
  };

  struct ModifiedPropertyInformation
    {
    vtkSMProxy* Proxy;
    const char* PropertyName;
    };

  struct LoadStateInformation
    {
    vtkPVXMLElement* RootElement;
    vtkSMProxyLocator* ProxyLocator;
    };

  struct StateChangedInformation
    {
    vtkSMProxy* Proxy;
    vtkPVXMLElement* StateChangeElement;
    };
//ETX

  // Description:
  // Get if there are any registered proxies that have their properties in
  // a modified state.
  int AreProxiesModified();

  // Description:
  // The server manager configuration XML may define <Hints /> element for
  // a proxy/property. Hints are metadata associated with the
  // proxy/property. The Server Manager does not (and should not) interpret
  // the hints. Hints provide a mechanism to add GUI pertinant information
  // to the server manager XML.  Returns the XML element for the hints
  // associated with this proxy/property, if any, otherwise returns NULL.
  vtkPVXMLElement* GetProxyHints(const char* xmlgroup, const char* xmlname);
  vtkPVXMLElement* GetPropertyHints(const char* groupName,
                                    const char* proxyName,
                                    const char* propertyName);

  // Description:
  // Check if UpdateInputProxies flag is set.
  // This is used after loading state or after instantiating
  // a compound proxy. This uses the "UpdateInputProxies" flag which
  // vtkSMProxy checks in UpdateVTKObjects() to call UpdateVTKObjects() on the input
  // proxies as well if the flag is set.
  vtkGetMacro(UpdateInputProxies, int);

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the major version number eg. if version is 2.9.1
  // this method will return 2.
  static int GetVersionMajor();

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the minor version number eg. if version is 2.9.1
  // this method will return 9.
  static int GetVersionMinor();

  // Description:
  // These methods can be used to obtain the ProxyManager version number.
  // Returns the patch version number eg. if version is 2.9.1
  // this method will return 1.
  static int GetVersionPatch();

  // Description:
  // Returns a string with the format "paraview version x.x.x, Date: YYYY-MM-DD"
  static const char* GetParaViewSourceVersion();

  // Description:
  // Register/UnRegister a selection model. A selection model can be typically
  // used by applications to keep track of active sources, filters, views etc.
  void RegisterSelectionModel(const char* name, vtkSMProxySelectionModel*);
  void UnRegisterSelectionModel(const char* name);

  // Description:
  // Get a registered selection model. Will return null if no such model is
  // registered.
  vtkSMProxySelectionModel* GetSelectionModel(const char* name);

  // Description:
  // ParaView has notion of "global properties". These are application wide
  // properties such as foreground color, text color etc. Changing values of
  // these properties affects all objects that are linked to these properties.
  // This class provides convenient API to setup/remove such links.
  void SetGlobalPropertiesManager(const char* name,
    vtkSMGlobalPropertiesManager*);
  void RemoveGlobalPropertiesManager(const char* name);

  // Description:
  // Accessors for global properties managers.
  unsigned int GetNumberOfGlobalPropertiesManagers();
  vtkSMGlobalPropertiesManager* GetGlobalPropertiesManager(unsigned int index);
  vtkSMGlobalPropertiesManager* GetGlobalPropertiesManager(const char* name);
  const char* GetGlobalPropertiesManagerName(vtkSMGlobalPropertiesManager*);

  // Description:
  // Provides access to the reader factory. Before using the reader factory, it
  // is essential that it's configured correctly.
  vtkGetObjectMacro(ReaderFactory, vtkSMReaderFactory);

  // Description:
  // Provides access to the writer factory. Before using the reader factory, it
  // is essential that it's configured correctly.
  vtkGetObjectMacro(WriterFactory, vtkSMWriterFactory);

  // Description:
  // Loads server-manager configuration xml.
  bool LoadConfigurationXML(const char* xmlcontents);

  // Description:
  // Get the proxy definition manager.
  // Proxy definition manager maintains all the information about proxy
  // definitions.
  vtkGetObjectMacro(ProxyDefinitionManager, vtkSMProxyDefinitionManager);

//BTX

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState();

  // Description:
  // This method is used to initialise the ProxyManager to the given state
  virtual void LoadState(const vtkSMMessage* msg, vtkSMStateLocator* locator);

  // Description:
  // This metod allow the creation of a proxy based on its full state.
  // Used in Undo/Redo to bring back a proxy to life or in collaboration mode,
  // where the message come from the server and the purpose is to create the SM
  // side when the PM side has already been created.
  // The User MUST delete the provided proxy otherwise it will live forever.
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.

  virtual vtkSMProxy* NewProxy( const vtkSMMessage* msg,
                                vtkSMStateLocator* locator,
                                bool definitionOnly = false);

  // Description:
  // Re-New a proxy based on its ID and its previous state.
  // This means that it will create a new vtkSMProxy or will
  // return NULL if that one already exist.
  //
  // WARNING:
  // - It is at the responsability at the caller to delete the proxy.
  virtual vtkSMProxy* ReNewProxy(vtkTypeUInt32 globalId,
                                 vtkSMStateLocator* locator);

protected:
  vtkSMProxyManager();
  ~vtkSMProxyManager();

  friend class vtkSMGlobalPropertiesManager;
  friend class vtkSMProxy;
  friend class vtkPVProxyDefinitionIterator;
  friend class vtkSMProxyIterator;
  friend class vtkSMProxyManagerObserver;

  // Description:
  // Given an XML element and group name create a proxy
  // and all of it's properties.
  vtkSMProxy* NewProxy(vtkPVXMLElement* element, const char* groupname,
                       const char* proxyname, const char* subProxyName = NULL);

  // Description:
  // Given the proxy name and group name, returns the XML element for
  // the proxy.
  vtkPVXMLElement* GetProxyElement( const char* groupName,
                                    const char* proxyName,
                                    const char* subProxyName = NULL);

  // Description:
  // Handles events.
  virtual void ExecuteEvent(vtkObject* obj, unsigned long event, void* data);

  // Description:
  // Removes a prototype. Used internally to cleanup obsolete prototypes.
  void RemovePrototype(const char* groupname, const char* proxyname);

  // Description:
  // Mark/UnMark a proxy as modified.
  void MarkProxyAsModified(vtkSMProxy*);
  void UnMarkProxyAsModified(vtkSMProxy*);

  // Description:
  // Save/Load registered link states.
  void SaveRegisteredLinks(vtkPVXMLElement* root);

  // Description:
  // Save global property managers.
  void SaveGlobalPropertiesManagers(vtkPVXMLElement* root);

  // Description:
  // Internal method to save server manager state in an XML
  // and return the created vtkPVXMLElement for it. The caller has
  // the responsibility of freeing the vtkPVXMLElement returned IF the
  // parentElement is NULL.
  vtkPVXMLElement* AddInternalState(vtkPVXMLElement* parentElement);

  // Recursively collects all proxies referred by the proxy in the set.
  void CollectReferredProxies( vtkSMProxyManagerProxySet& setOfProxies,
                               vtkSMProxy* proxy);

  int UpdateInputProxies;

  vtkSMReaderFactory* ReaderFactory;
  vtkSMWriterFactory* WriterFactory;
  vtkSMProxyDefinitionManager* ProxyDefinitionManager;
  vtkSMSession* Session;
  vtkEventForwarderCommand* Forwarder;
  vtkSMPipelineState* PipelineState;
private:
  vtkSMProxyManagerInternals* Internals;
  vtkSMProxyManagerObserver* Observer;

private:
  vtkSMProxyManager(const vtkSMProxyManager&); // Not implemented
  void operator=(const vtkSMProxyManager&); // Not implemented
//ETX
};

#endif
