/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxy - proxy for a VTK object(s) on a server
// .SECTION Description
// vtkSMProxy manages VTK object(s) that are created on a server 
// using the proxy pattern. The managed object is manipulated through 
// properties. 
// The type of object created and managed by vtkSMProxy is determined
// by the VTKClassName variable. The object is managed by getting the desired
// property from the proxy, changing it's value and updating the server
// with UpdateVTKObjects().
// A proxy can be composite. Sub-proxies can be added by the proxy 
// manager. This is transparent to the user who sees all properties
// as if they belong to the root proxy.
//
// A proxy keeps an iVar ConnectionID. This is the connection ID for the
// connection on which this proxy exists. Currently, since a ParaView
// client is connected to 1 and only 1 server. This ID is
// insignificant. However, it provides the ground work to enable a client
// to connect with multiple servers.  ConnectionID must be set immediately
// after instantiating the proxy (if at all).  Chanding the ConnectionID
// after that can be dangerous.
// 
// When defining a proxy in the XML configuration file,
// to derrive the property interface from another proxy definition,
// we can use attributes "base_proxygroup" and "base_proxyname" which 
// identify the proxy group and proxy name of another proxy. Base interfaces
// can be defined recursively, however care must be taken to avoid cycles.
// 
// There are several special XML features available for subproxies.
// \li 1) It is possible to share properties among subproxies.
//    eg.
//    \code
//    <Proxy name="Display" class="Alpha">
//      <SubProxy>
//        <Proxy name="Mapper" class="vtkPolyDataMapper">
//          <InputProperty name="Input" ...>
//            ...
//          </InputProperty>
//          <IntVectorProperty name="ScalarVisibility" ...>
//            ...
//          </IntVectorProperty>
//            ...
//        </Proxy>
//      </SubProxy>
//      <SubProxy>
//        <Proxy name="Mapper2" class="vtkPolyDataMapper">
//          <InputProperty name="Input" ...>
//            ...
//          </InputProperty>
//          <IntVectorProperty name="ScalarVisibility" ...>
//            ...
//          </IntVectorProperty>
//            ...
//        </Proxy>
//        <ShareProperties subproxy="Mapper">
//          <Exception name="Input" />
//        </ShareProperties>
//      </SubProxy>
//    </Proxy>
//    \endcode
//    Thus, subproxies Mapper and Mapper2 share the properties that are 
//    common to both; except those listed as exceptions using the "Exception" 
//    tag.
//
// \li 2) It is possible for a subproxy to use proxy definition defined elsewhere
//     by identifying the interface with attribues "proxygroup" and "proxyname".
//     eg.
//     \code
//     <SubProxy>
//       <Proxy name="Mapper" proxygroup="mappers" proxyname="PolyDataMapper" />
//     </SubProxy>
//     \endcode
//
// \li 3) It is possible to scope the properties exposed by a subproxy and expose
//     only a fixed set of properties to be accessible from outside. Also,
//     while exposing the property, it can be exposed with a different name. 
//     eg.
//     \code
//     <Proxy name="Alpha" ....>
//       ....
//       <SubProxy>
//         <Proxy name="Mapper" proxygroup="mappers" proxyname="PolyDataMapper" />
//         <ExposedProperties>
//           <Property name="LookupTable" exposed_name="MapperLookupTable" />
//         </ExposedProperties>
//       </SubProxy>
//     </Proxy>
//     \endcode
//     Here, for the proxy Alpha, the property with the name LookupTable from its 
//     subproxy "Mapper" can be obtained by calling GetProperty("MapperLookupTable")
//     on an instance of the proxy Alpha. "exposed_name" attribute is optional, if 
//     not specified, then the "name" is used as the exposed property name.
//     Properties that are not exposed are treated as
//     non-saveable and non-animateable (see vtkSMProperty for details).
//     Exposed property restrictions only work when 
//     using the GetProperty on the container proxy (in this case Alpha) or
//     using the PropertyIterator obtained from the container proxy. If one
//     is to some how obtain a pointer to the subproxy and call GetProperty on 
//     it (or get a PropertyIterator for the subproxy), the properties exposed 
//     by the container class are no longer applicable.
//     If two exposed properties are exposed with the same name, then a Warning is
//     flagged -- only one of the two exposed properties will get exposed. 
//
// .SECTION See Also
// vtkSMProxyManager vtkSMProperty vtkSMSourceProxy vtkSMPropertyIterator

#ifndef __vtkSMProxy_h
#define __vtkSMProxy_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

//BTX
struct vtkSMProxyInternals;
//ETX
class vtkGarbageCollector;
class vtkPVXMLElement;
class vtkSMDocumentation;
class vtkSMProperty;
class vtkSMPropertyIterator;
class vtkSMProxyManager;
class vtkSMProxyObserver;
class vtkClientServerStream;
class vtkSMProxyLocator;

class VTK_EXPORT vtkSMProxy : public vtkSMObject
{
public:
  static vtkSMProxy* New();
  vtkTypeRevisionMacro(vtkSMProxy, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property with the given key (name). The name can then
  // be used to retrieve the property with GetProperty(). If a
  // property with the given name has been added before, it will
  // be replaced. This includes properties in sub-proxies.
  void AddProperty(const char* name, vtkSMProperty* prop);

  // Description:
  // Return the property with the given name. If no property is found
  // NULL is returned.
  vtkSMProperty* GetProperty(const char* name) 
    {
      return this->GetProperty(name, 0);
    }

  // Description:
  // Given a property pointer, returns the name that was used
  // to add it to the proxy. Returns NULL if the property is
  // not in the proxy. If the property belongs to a sub-proxy,
  // it returns the exposed name or NULL if the property is not
  // exposed.
  const char* GetPropertyName(vtkSMProperty* prop);

  // Description:
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  // Description:
  // Update the value of one property (pushed to the server) if it is
  // modified.  If the object has not been created, it will be created
  // first. If force is true, the property is pushed even if it not
  // modified. Return true if UpdateProperty pushes the property value.
  bool UpdateProperty(const char* name)
    {
      return this->UpdateProperty(name, 0);
    }
  bool UpdateProperty(const char* name, int force);

  // Description:
  // Convenience method equivalent to UpdateProperty(name, 1).
  void InvokeCommand(const char* name)
    {
      this->UpdateProperty(name, 1);
    }

  // Description:
  // Calls UpdateVTKObjects() on self and all proxies that depend
  // on this proxy (through vtkSMProxyProperty properties). It will
  // traverse the dependence tree and update starting from the source.
  // This allows instantiating a whole pipeline (including connectivity)
  // without having to worry about the order. Here is how to do it:
  // \li * Create all proxies
  // \li * Set all property values - make sure that input properties
  //      do not auto update by calling 
  //      vtkSMInputProperty::SetInputsUpdateImmediately(0); 
  // \li * Call UpdateSelfAndAllInputs() on either all proxies or
  //   one that depends on all others (usually one or more DisplayWindows)
  // \li * If necessary vtkSMInputProperty::SetInputsUpdateImmediately(1); 
  virtual void UpdateSelfAndAllInputs();

  // Description:
  // Returns the type of object managed by the proxy.
  vtkGetStringMacro(VTKClassName);

  // Description:
  // the type of object created by the proxy.
  // This is used only when creating the server objects. Once the server
  // object(s) have been created, changing this has no effect.
  vtkSetStringMacro(VTKClassName);

  // Description:
  // Overloaded to break the reference loop caused by the fact that
  // proxies store their own ClientServer ids.
  // Overloaded also for garbage collection.
  virtual void UnRegister(vtkObjectBase* obj);

  // Description:
  // Overloaded also for garbage collection.
  virtual void Register(vtkObjectBase* obj);

  // Description:
  // Returns the id of a server object.
  int GetIDAsInt()
  {
    vtkClientServerID id = this->GetID();
    return id.ID;
  }

//BTX
  // Description:
  // Returns the id of a server object.
  vtkClientServerID GetID();

  // Description:
  // Returns the Self ID of the proxy.
  // If the SelfID is not assigned yet, then this method will assign this proxy
  // a unique SelfID on the interpretor for the connection on which this 
  // proxy exists i.e. this->ConnectionID.
  vtkClientServerID GetSelfID();
//ETX

  // Description:
  // Returns a new (initialized) iterator of the properties.
  virtual vtkSMPropertyIterator* NewPropertyIterator();

  // Description:
  // Returns the number of consumers. Consumers are proxies
  // that point to this proxy through a property (usually 
  // vtkSMProxyProperty)
  unsigned int GetNumberOfConsumers();

  // Description:
  // Returns the consumer of given index. Consumers are proxies
  // that point to this proxy through a property (usually 
  // vtkSMProxyProperty)
  vtkSMProxy* GetConsumerProxy(unsigned int idx);

  // Description:
  // Returns the corresponding property of the consumer of given 
  // index. Consumers are proxies that point to this proxy through 
  // a property (usually vtkSMProxyProperty)
  vtkSMProperty* GetConsumerProperty(unsigned int idx);

  // Description:
  // Returns the number of proxies this proxy depends on (uses or is
  // connected to through the pipeline).
  unsigned int GetNumberOfProducers();

  // Description:
  // Returns a proxy this proxy depends on, given index.
  vtkSMProxy* GetProducerProxy(unsigned int idx);

  // Description:
  // Returns the property holding a producer proxy given an index. Note
  // that this is a property of this proxy and it points to the producer
  // proxy.
  vtkSMProperty* GetProducerProperty(unsigned int idx);

  // Description:
  // Assigned by the XML parser. The name assigned in the XML
  // configuration. Can be used to figure out the origin of the
  // proxy.
  vtkGetStringMacro(XMLName);

  // Description:
  // Assigned by the XML parser. The group in the XML configuration that
  // this proxy belongs to. Can be used to figure out the origin of the
  // proxy.
  vtkGetStringMacro(XMLGroup);

  // Description:
  // Assigned by the XML parser. The label assigned in the XML
  // configuration. This is a more user-friendly name
  // for the proxy, although it's cannot be used to locate the
  // proxy.
  vtkGetStringMacro(XMLLabel);

  // Description:
  // Updates all property informations by calling UpdateInformation()
  // and populating the values. It also calls UpdateDependentDomains()
  // on all properties to make sure that domains that depend on the
  // information are updated.
  virtual void UpdatePropertyInformation();

  // Description:
  // Similar to UpdatePropertyInformation() but updates only the given property.
  // If the property does not belong to the proxy, the call is ignored.
  virtual void UpdatePropertyInformation(vtkSMProperty* prop);

  // Description:
  // Marks all properties as modified.  This will cause them all to be sent
  // to be sent on the next call to UpdateVTKObjects.  This method is
  // useful when the proxy is first created to make sure that the default
  // property values in the properties is synced with the values in the
  // actual objects.
  virtual void MarkAllPropertiesAsModified();
  
  // Description:
  // Set server ids on self and sub-proxies.
  virtual void SetServers(vtkTypeUInt32 servers);
 
  // Description:
  // Return the servers.
  vtkTypeUInt32 GetServers();

  // Description:
  // Set the server connection ID on self and sub-proxies.
  virtual void SetConnectionID(vtkIdType id);

  // Description:
  // Returns the server connection ID.
  vtkIdType GetConnectionID();

//BTX
  // Description:
  // Flags used for the proxyPropertyCopyFlag argument to the Copy method.
  enum
    {
    COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE=0,
    COPY_PROXY_PROPERTY_VALUES_BY_CLONING
    };
//ETX

  // Description:
  // Copies values of all the properties and sub-proxies from src.
  // \b NOTE: This does NOT create properties and sub-proxies. Only
  // copies values. Mismatched property and sub-proxy pairs are
  // ignored.
  // Properties of type exceptionClass are not copied. This
  // is usually vtkSMInputProperty.
  // proxyPropertyCopyFlag specifies how the values for vtkSMProxyProperty
  // and its subclasses are copied over: by reference or by 
  // cloning (ie. creating new instances of the value proxies and 
  // synchronizing their values).
  void Copy(vtkSMProxy* src);
  void Copy(vtkSMProxy* src, const char* exceptionClass);
  virtual void Copy(vtkSMProxy* src, const char* exceptionClass, 
    int proxyPropertyCopyFlag);
  
  // Description:
  // Calls MarkDirty() and invokes ModifiedEvent.
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

  // Description:
  // Returns the self ID as string. If the name was overwritten
  // with SetName(), it returns that instead.
  const char* GetSelfIDAsString();

  // Description:
  // Returns the documentation for this proxy.
  vtkGetObjectMacro(Documentation, vtkSMDocumentation);

  // Description:
  // The server manager configuration XML may define <Hints /> element for a 
  // proxy. Hints are metadata associated with the proxy. The Server Manager
  // does not (and should not) interpret the hints. Hints provide a mechanism
  // to add GUI pertinant information to the server manager XML.
  // Returns the XML element for the hints associated with this proxy,
  // if any, otherwise returns NULL.  
  vtkGetObjectMacro(Hints, vtkPVXMLElement);

  // Description:
  // Retuns if the VTK objects for this proxy have been created.
  vtkGetMacro(ObjectsCreated, int);

  // Description:
  // Given a source proxy, makes this proxy point to the same server-side
  // object (with a new id). This method copies connection id as well as
  // server ids. This method can be called only once on an uninitialized
  // proxy (CreateVTKObjects() also initialized a proxy) This is useful to
  // make two (or more) proxies represent the same VTK object. This method
  // does not copy IDs for any subproxies.
  void InitializeAndCopyFromProxy(vtkSMProxy* source);

//BTX
  // Description:
  // Initializes this proxy with the given id for an already created server
  // side object. Make sure to set Servers before calling this method.
  // This method will call CreateVTKObjects() on all sub-proxies. This
  // method can be called only once on an uninitialized proxy
  // (CreateVTKObjects() also initialized a proxy)
  virtual void InitializeWithID(vtkClientServerID id);

  // Description:
  // Initializes this proxy with a new id referring to the same object on the
  // server as of the given id already created server
  // side object. Make sure to set Servers before calling this method.
  // This method will call CreateVTKObjects() on all sub-proxies. This
  // method can be called only once on an uninitialized proxy
  // (CreateVTKObjects() also initialized a proxy)
  virtual void InitializeAndCopyFromID(vtkClientServerID id);
//ETX

  // Description:
  // Saves the state of the proxy. This state can be reloaded
  // to create a new proxy that is identical the present state of this proxy.
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);

  // Description:
  // Loads the proxy state from the XML element. Returns 0 on failure.
  // \c locator is used to locate other proxies that may be referred to in the
  // state XML (which happens in case of properties of type vtkSMProxyProperty
  // or subclasses). If locator is NULL, then such properties are left
  // unchanged.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  // Description:
  // Dirty means this algorithm will execute during next update.
  // This all marks all consumers as dirty.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Returns the client side object associated with the VTKObjectID (if any).
  // Returns 0 otherwise.
  vtkObjectBase* GetClientSideObject();

//BTX
protected:
  vtkSMProxy();
  ~vtkSMProxy();

  // Description:
  // Calls MarkDirty() on all consumers.
  virtual void MarkConsumersAsDirty(vtkSMProxy* modifiedProxy);


  // Description:
  // Overloaded for garbage collection.
  virtual void ReportReferences(vtkGarbageCollector* collector);

  // Description:
  // Expose a subproxy property from the base proxy. The property with the name
  // "property_name" on the subproxy with the name "subproxy_name" is exposed 
  // with the name "exposed_name". 
  // If the overrideOK flag is set, then no warning is printed when a new 
  // exposed property replaces a preexisting one.
  void ExposeSubProxyProperty(const char* subproxy_name, 
                              const char* property_name,
                              const char* exposed_name,
                              int overrideOK = 0);


  // Description:
  // Pushes the property. For non update-self properties, the command is
  // collected in the stream and not pushed directly.
  bool UpdatePropertyInternal(const char* name, bool force,
    vtkClientServerStream& stream);

  // Description:
  // These classes have been declared as friends to minimize the
  // public interface exposed by vtkSMProxy. Each of these classes
  // use a small subset of protected methods. This should be kept
  // as such.
  friend class vtkSMCameraLink;
  friend class vtkSMCompoundProxy;
  friend class vtkSMCompoundSourceProxy;
  friend class vtkSMInputProperty;
  friend class vtkSMOrderedPropertyIterator;
  friend class vtkSMPart;
  friend class vtkSMProperty;
  friend class vtkSMPropertyIterator;
  friend class vtkSMProxyManager;
  friend class vtkSMProxyObserver;
  friend class vtkSMProxyProperty;
  friend class vtkSMProxyRegisterUndoElement;
  friend class vtkSMProxyUnRegisterUndoElement;
  friend class vtkSMSourceProxy;
  friend class vtkSMUndoRedoStateLoader;
  // -- PVEE only
  friend class vtkWSMApplication;

  // Description:
  // Assigned by the XML parser. The name assigned in the XML
  // configuration. Can be used to figure out the origin of the
  // proxy.
  vtkSetStringMacro(XMLName);

  // Description:
  // Assigned by the XML parser. The group in the XML configuration that
  // this proxy belongs to. Can be used to figure out the origin of the
  // proxy.
  vtkSetStringMacro(XMLGroup);

  // Description:
  // Assigned by the XML parser. The label assigned in the XML
  // configuration. This is a more user-friendly name
  // for the proxy, although it's cannot be used to locate the
  // proxy.
  vtkSetStringMacro(XMLLabel);

  // Description:
  // It is possible to set the SelfID for a proxy. However then the setter
  // has the responsiblity to ensure that the ID is going to be unique 
  // for the lifetime of the proxy. Also the SelfID can be set, only before
  // an ID was assigned to the proxy. 
  void SetSelfID(vtkClientServerID id);

  // Description:
  // Returns the Self ID of the proxy.
  // Note: unlike GetSelfID(), this method simply return SelfID without assigning it 
  // if the SelfID is not assigned yet, so the SelfID.ID could be zero.
  vtkClientServerID GetSelfIDInternal()
    { return this->SelfID; }
  
  // Description:
  // Given a class name (by setting VTKClassName) and server ids (by
  // setting ServerIDs), this methods instantiates the objects on the
  // server(s)
  virtual void CreateVTKObjects();

  // Description:
  // This method is complementary to CreateVTKObjects() when we want the
  // proxy to reuse the VTK object ID are defined. When reviving a proxy
  // on the client, this method creates new client-side objects and reuses
  // the server side objects.  When reviving a proxy on the server, it
  // reuses the server objects while creating new "client-only" objects on
  // the server itself.
  virtual void ReviveVTKObjects();

  // Description:
  // UnRegister all managed objects. This also resets the ID list.
  // However, it does not remove the properties.
  void UnRegisterVTKObjects();

  // Description:
  // Server IDs determine on which server(s) the VTK objects are
  // instantiated. Use the following methods to set/get the server
  // IDs. Server IDs have to be set before the object is created.
  // Changing them after creation has no effect.
  // See vtkProcessModule.h for a list of all server types.
  // To add a server, OR it's value with the servers ivar.
  // Set server ids on self
  void SetServersSelf(vtkTypeUInt32 servers);

  // Description:
  // Set the server connection id on self.
  void SetConnectionIDSelf(vtkIdType id);

  // Description:
  // Cleanup code. Remove all observers from all properties assigned to
  // this proxy.  Called before deleting properties.
  // This also removes observers on subproxies.
  void RemoveAllObservers();

  // Description:
  // Note on property modified flags:
  // The modified flag of each property associated with a proxy is
  // stored in the proxy object instead of in the property itself.
  // Here is a brief explanation of how modified flags are used:
  // \li 1. When a property is modified, the modified flag is set
  // \li 2. In UpdateVTKObjects(), the proxy visits all properties and
  //    calls AppendCommandToStream() on each modified property.
  //    It also resets the modified flag.
  //
  // The reason why the modified flag is stored in the proxy instead
  // of property is in item 2 above. If multiple proxies were sharing the same
  // property, the first one would reset the modified flag in
  // UpdateVTKObjects() and then others would not call AppendCommandToStream()
  // in their turn. Therefore, each proxy has to keep track of all
  // properties it updated.
  // This is done by adding observers to the properties. When a property
  // is modified, it invokes all observers and the observers set the
  // appropriate flags in the proxies. 
  // Changes the modified flag of a property. Used by the observers
  virtual void SetPropertyModifiedFlag(const char* name, int flag);

  // Description:
  // Add a property to either self (subProxyName = 0) or a sub-proxy.
  // \b IMPORTANT: If subProxyName = 0, AddProperty() checks for a
  // proxy with the given name in self and all sub-proxies, if one
  // exists, it replaces it. In this special case, it is possible for
  // the property to be added to a sub-proxy as opposed to self.
  void AddProperty(const char* subProxyName,
                   const char* name, 
                   vtkSMProperty* prop);

  // Description:
  // Remove a property from the list.
  // If selfOnly is true, this method will not traverse into the subproxies.
  void RemoveProperty(const char* name);

  // Description:
  // Add a property to self.
  void AddPropertyToSelf(const char* name, vtkSMProperty* prop);

  // Description:
  // Add a sub-proxy.
  // If the overrideOK flag is set, then no warning is printed when a new 
  // subproxy replaces a preexisting one.
  void AddSubProxy(const char* name, vtkSMProxy* proxy,
                   int overrideOK=0);

  // Description:
  // Remove a sub-proxy.
  void RemoveSubProxy(const char* name);

  // Description:
  // Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
  vtkSMProxy* GetSubProxy(const char* name);

  // Description:
  // Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
  vtkSMProxy* GetSubProxy(unsigned int index);

  // Description:
  // Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
  // not exist.
  const char* GetSubProxyName(unsigned int index);

  // Description:
  // Returns the name used to store sub-proxy. Returns 0 is the sub-proxy
  // does not exist.
  const char* GetSubProxyName(vtkSMProxy*);

  // Description:
  // Returns the number of sub-proxies.
  unsigned int GetNumberOfSubProxies();

  // Description:
  // Save relevant information about subproxies. Note that we don't need
  // to save subproxy property information since only 
  // exposed properties are saved.
  void SaveSubProxyState(vtkPVXMLElement* root);

  // Description:
  // Load the subproxy state.
  void LoadSubProxyState(vtkPVXMLElement* subproxyElement, 
    vtkSMProxyLocator* locator);

  // Description:
  // Called by a proxy property, this adds the property,proxy
  // pair to the list of consumers.
  void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  // Description:
  // Remove the property,proxy pair from the list of consumers.
  void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  // Description:
  // Remove all consumers.
  void RemoveAllConsumers();

  // Description:
  // Called by an proxy/input property to add property, proxy pair
  // to the list of producers.
  void AddProducer(vtkSMProperty* property, vtkSMProxy* proxy);
  
  // Description:
  // Remove the property,proxy pair from the list of producers.
  void RemoveProducer(vtkSMProperty* property, vtkSMProxy* proxy);

  // Description:
  // This method is called after the algorithm(s) (if any) associated
  // with this proxy execute. Subclasses overwrite this method to
  // add necessary functionality.
  virtual void PostUpdateData();

  // Description:
  // If a proxy is deprecated, prints a warning.
  bool WarnIfDeprecated();
  
  // When an algorithm proxy is marked modified, NeedsUpdate is
  // set to true. In PostUpdateData(), NeedsUpdate is set to false.
  // This is used to keep track of data information validity.
  bool NeedsUpdate;
  
  // Description:
  // Creates a new proxy and initializes it by calling ReadXMLAttributes()
  // with the right XML element.
  vtkSMProperty* NewProperty(const char* name);
  vtkSMProperty* NewProperty(const char* name, vtkPVXMLElement* propElement);

  // Description:
  // Return a property of the given name from self or one of
  // the sub-proxies. If selfOnly is set, the sub-proxies are
  // not checked.
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  // Description:
  // Used by ReadXMLAttributes() to read the very core attributes such as
  // documentaion, hints, name etc.
  void ReadCoreXMLAttributes(vtkPVXMLElement* element);

  // Description:
  // Handle events fired by subproxies.
  virtual void ExecuteSubProxyEvent(vtkSMProxy* o, unsigned long event, 
    void* data);

  // Description:
  // This method simply iterates over subproxies and calls 
  // UpdatePipelineInformation() on them. vtkSMSourceProxy overrides this method
  // (makes it public) and updates the pipeline information.
  virtual void UpdatePipelineInformation();

  // Description:
  // Internal UpdateVTKObjects() which collapses all server-side sends into the
  // stream.
  virtual void UpdateVTKObjects(vtkClientServerStream& stream);

  // Description:
  // Loads the revival state for the proxy.
  // RevivalState includes the entire state saved by calling 
  // SaveState() as well additional information such as server side
  // object ID. This makes it possible to restore the servermanager state
  // while reusing server side object id.
  virtual int LoadRevivalState(vtkPVXMLElement* revivalElement);

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);
 
  virtual int CreateSubProxiesAndProperties(vtkSMProxyManager* pm, 
    vtkPVXMLElement *element);

  char* Name;
  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  char* XMLLabel;
  int ObjectsCreated;
  vtkTypeUInt32 Servers;
  int DoNotUpdateImmediately;
  int DoNotModifyProperty;

  // Description:
  // Avoids calls to UpdateVTKObjects in UpdateVTKObjects.
  // UpdateVTKObjects call it self recursively until no
  // properties are modified.
  int InUpdateVTKObjects;

  // Description:
  // Flag used to help speed up UpdateVTKObjects and ArePropertiesModified
  // calls.
  int SelfPropertiesModified;

  // Description:
  // Indicates if any properties are modified.
  int ArePropertiesModified(int selfOnly = 0);

  void SetHints(vtkPVXMLElement* hints);
  void SetDeprecated(vtkPVXMLElement* deprecated);

  void SetXMLElement(vtkPVXMLElement* element);
  vtkPVXMLElement* XMLElement;

  // Description:
  // This method saves state information about the proxy
  // which can be used to revive the proxy using server side objects
  // already present. RevivalState includes the entire state saved by calling 
  // SaveState() as well additional information such as server side
  // object ID. This makes it possible to restore the servermanager state
  // while reusing server side object ids.
  virtual vtkPVXMLElement* SaveRevivalState(vtkPVXMLElement* root);

  void SetupSharedProperties(vtkSMProxy* subproxy, vtkPVXMLElement *element);
  void SetupExposedProperties(const char* subproxy_name, vtkPVXMLElement *element);
  
  int CreateProxyHierarchy(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  // Description:
  // This ID is the connection ID to the server on which this
  // proxy exists, if at all. By default, it is the RootServerConnectionID.
  vtkIdType ConnectionID;

  vtkSMDocumentation* Documentation;
  vtkPVXMLElement* Hints;
  vtkPVXMLElement* Deprecated;

  // Flag used to break consumer loops.
  int InMarkModified;

  // Description:
  // Client/server id of the VTK object this is a proxy of.
  vtkClientServerID VTKObjectID;

private:
  vtkSMProxyInternals* Internals;
  vtkSMProxyObserver* SubProxyObserver;

  // Description:
  // SelfID is private to avoid direct access by subclasses.
  // They must use GetSelfID().
  vtkClientServerID SelfID; 

  // Description:
  // PVEE only
  // DO NOT USE THIS. THIS IS TEMPORARY AND TO BE USED
  // IN PVEE ONLY
  // A proxy can be assigned a name. The name is used to
  // indentify the proxy when saving ServerManager state.
  // By default the name is set to the SelfID of the proxy.
  vtkSetStringMacro(Name);

  void RegisterSelfID();

  vtkSMProxy(const vtkSMProxy&); // Not implemented
  void operator=(const vtkSMProxy&); // Not implemented
//ETX
};

#endif
