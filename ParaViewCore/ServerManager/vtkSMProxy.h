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

#include "vtkSMRemoteObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

//BTX
struct vtkSMProxyInternals;
//ETX
class vtkClientServerStream;
class vtkPVInformation;
class vtkPVXMLElement;
class vtkSMDocumentation;
class vtkSMProperty;
class vtkSMPropertyIterator;
class vtkSMProxyLocator;
class vtkSMProxyManager;
class vtkSMProxyObserver;

class VTK_EXPORT vtkSMProxy : public vtkSMRemoteObject
{
public:
  // ----------- DEPRECATED API --------------
  VTK_LEGACY(void SetServers(vtkTypeUInt32));
  VTK_LEGACY(vtkTypeUInt32 GetServers());
  VTK_LEGACY(vtkTypeUInt32 GetConnectionID());
  VTK_LEGACY(void SetConnectionID(vtkTypeUInt32));
  // ----------- DEPRECATED API --------------

public:
  static vtkSMProxy* New();
  vtkTypeMacro(vtkSMProxy, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the location where the underlying VTK-objects are created. The
  // value can be contructed by or-ing vtkSMSession::ServerFlags
  virtual void SetLocation(vtkTypeUInt32);

  // Description:
  // Return the property with the given name. If no property is found
  // NULL is returned.
  virtual vtkSMProperty* GetProperty(const char* name)
    { return this->GetProperty(name, /*self-only*/ 0); }

  // Description:
  // Return a property of the given name from self or one of
  // the sub-proxies. If selfOnly is set, the sub-proxies are
  // not checked.
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

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
  // Returns the type of object managed by the proxy.
  vtkGetStringMacro(VTKClassName);

  // Description:
  // the type of object created by the proxy.
  // This is used only when creating the server objects. Once the server
  // object(s) have been created, changing this has no effect.
  vtkSetStringMacro(VTKClassName);

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

  // Description:
  // Dirty means this algorithm will execute during next update.
  // This all marks all consumers as dirty.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

  // Description:
  // Returns the client side object associated with the VTKObjectID (if any).
  // Returns 0 otherwise.
  vtkObjectBase* GetClientSideObject();

  // Description:
  // Gathers information about this proxy.
  // On success, the \c information object is filled up with details about the
  // VTK object.
  bool GatherInformation(vtkPVInformation* information);

  // Description:
  // Saves the state of the proxy. This state can be reloaded
  // to create a new proxy that is identical the present state of this proxy.
  // The resulting proxy's XML hieratchy is returned, in addition if the root
  // argument is not NULL then it's also inserted as a nested element.
  // This call saves all a proxy's properties, including exposed properties
  // and sub-proxies. More control is provided by the following overload.
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root);
  // Description:
  // The iterator is use to filter the property available on the given proxy
  virtual vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter);

  // Description:
  // Loads the proxy state from the XML element. Returns 0 on failure.
  // \c locator is used to locate other proxies that may be referred to in the
  // state XML (which happens in case of properties of type vtkSMProxyProperty
  // or subclasses). If locator is NULL, then such properties are left
  // unchanged.
  virtual int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  // Description:
  // Allow user to set the remote object to be discard for Undo/Redo
  // action. By default, any remote object is Undoable.
  // This override the RemoteObject ones to propagate the flag to the sub-proxy
  virtual void PrototypeOn();
  virtual void PrototypeOff();
  virtual void SetPrototype(bool undo);

  // Description:
  // This method call UpdateVTKObjects on the current pipeline by starting at
  // the sources. The sources are found by getting the Input of all the filter
  // along the pipeline.
  void UpdateSelfAndAllInputs();

//BTX

  // Description:
  // This method return the full object state that can be used to create that
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState();

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMStateLocator* locator,
                          bool definitionOnly = false);

protected:
  vtkSMProxy();
  ~vtkSMProxy();

  // Description:
  // Invoke that takes a vtkClientServerStream as the argument.
  void ExecuteStream(const vtkClientServerStream& msg,
                     bool ignore_errors = false,
                     vtkTypeUInt32 location = 0);

  // Description:
  // Get the last result
  virtual const vtkClientServerStream& GetLastResult();
  virtual const vtkClientServerStream& GetLastResult(vtkTypeUInt32 location);

  // Description:
  // Add a property with the given key (name). The name can then
  // be used to retrieve the property with GetProperty(). If a
  // property with the given name has been added before, it will
  // be replaced. This includes properties in sub-proxies.
  virtual void AddProperty(const char* name, vtkSMProperty* prop);


  // Description:
  // Calls MarkDirty() on all consumers.
  virtual void MarkConsumersAsDirty(vtkSMProxy* modifiedProxy);

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
  friend class vtkSMNamedPropertyIterator;
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
  // Assigned by the XML parser. It is used to figure out the origin
  // of the definition of that proxy.
  // By default, it stay NULL, only in-line subProxy do specify
  // this field so when the definition is sent to the server, it can
  // retreive the in-line definition of that proxy.
  vtkSetStringMacro(XMLSubProxyName);

  // Description:
  // Given a class name (by setting VTKClassName) and server ids (by
  // setting ServerIDs), this methods instantiates the objects on the
  // server(s)
  virtual void CreateVTKObjects();

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
  // Called by a proxy property, this adds the property,proxy
  // pair to the list of consumers.
  virtual void AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  // Description:
  // Remove the property,proxy pair from the list of consumers.
  virtual void RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy);

  // Description:
  // Remove all consumers.
  virtual void RemoveAllConsumers();

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

  // Description:
  // This method simply iterates over subproxies and calls 
  // UpdatePipelineInformation() on them. vtkSMSourceProxy overrides this method
  // (makes it public) and updates the pipeline information.
  virtual void UpdatePipelineInformation();
  
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
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);
  void SetupExposedProperties(const char* subproxy_name, vtkPVXMLElement *element);
  void SetupSharedProperties(vtkSMProxy* subproxy, vtkPVXMLElement *element);

  // Description:
  // Expose a subproxy property from the base proxy. The property with the name
  // "property_name" on the subproxy with the name "subproxy_name" is exposed
  // with the name "exposed_name".
  // If the overrideOK flag is set, then no warning is printed when a new
  // exposed property replaces a preexisting one.
  void ExposeSubProxyProperty(const char* subproxy_name,
    const char* property_name, const char* exposed_name, int overrideOK = 0);

  // Description:
  // Handle events fired by subproxies.
  virtual void ExecuteSubProxyEvent(vtkSMProxy* o, unsigned long event, void* data);

  virtual int CreateSubProxiesAndProperties(vtkSMProxyManager* pm,
    vtkPVXMLElement *element);

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty* prop=NULL);

  // Description:
  // SIClassName identifies the classname for the helper on the server side.
  vtkSetStringMacro(SIClassName);
  vtkGetStringMacro(SIClassName);
  char* SIClassName;


  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  char* XMLLabel;
  char* XMLSubProxyName;
  int ObjectsCreated;
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
  bool PropertiesModified;

  // Description:
  // Indicates if any properties are modified.
  bool ArePropertiesModified();

  void SetHints(vtkPVXMLElement* hints);
  void SetDeprecated(vtkPVXMLElement* deprecated);

  void SetXMLElement(vtkPVXMLElement* element);
  vtkPVXMLElement* XMLElement;

  vtkSMDocumentation* Documentation;
  vtkPVXMLElement* Hints;
  vtkPVXMLElement* Deprecated;

  // Cached version of State
  vtkSMMessage* State;

  // Flag used to break consumer loops.
  int InMarkModified;

protected:
  vtkSMProxyInternals* Internals;
  vtkSMProxyObserver* SubProxyObserver;
  vtkSMProxy(const vtkSMProxy&); // Not implemented
  void operator=(const vtkSMProxy&); // Not implemented
//ETX
};

//BTX

// This defines a manipulator for the vtkClientServerStream that can be used on
// the to indicate to the interpreter that the placeholder is to be replaced by
// the vtkSIProxy instance for the given vtkSMProxy instance.
// e.g.
// <code>
// vtkClientServerStream stream;
// stream << vtkClientServerStream::Invoke
//        << SIPROXY(proxyA)
//        << "MethodName"
//        << vtkClientServerStream::End;
// </code>
// Will result in calling the vtkSIProxy::MethodName() when the stream in
// interpreted.
class VTK_EXPORT SIPROXY : public SIOBJECT
{
public:
  SIPROXY(vtkSMProxy* proxy) : SIOBJECT (proxy) { }
};

// This defines a manipulator for the vtkClientServerStream that can be used on
// the to indicate to the interpreter that the placeholder is to be replaced by
// the vtkObject instance for the given vtkSMProxy instance.
// e.g.
// <code>
// vtkClientServerStream stream;
// stream << vtkClientServerStream::Invoke
//        << VTKOBJECT(proxyA)
//        << "MethodName"
//        << vtkClientServerStream::End;
// </code>
// Will result in calling the vtkClassName::MethodName() when the stream in
// interpreted where vtkClassName is the type for the VTKObject which the proxyA
// represents.
class VTK_EXPORT VTKOBJECT
{
  vtkSMProxy* Reference;
  friend VTK_EXPORT vtkClientServerStream& operator<<(
    vtkClientServerStream& stream, const VTKOBJECT& manipulator);
public:
  VTKOBJECT(vtkSMProxy* proxy) : Reference(proxy) {}
};
VTK_EXPORT vtkClientServerStream& operator<<(
  vtkClientServerStream& stream, const VTKOBJECT& manipulator);

//ETX
#endif
