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
// .SECTION See Also
// vtkSMProxyManager vtkSMProperty vtkSMSourceProxy vtkSMPropertyIterator

#ifndef __vtkSMProxy_h
#define __vtkSMProxy_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

//BTX
struct vtkSMProxyInternals;
//ETX
class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMPropertyIterator;
class vtkSMProxyManager;

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
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  // Description:
  // Calls UpdateVTKObjects() on self and all proxies that depend
  // on this proxy (through vtkSMProxyProperty properties). It will
  // traverse the dependence tree and update starting from the source.
  // This allows instantiating a whole pipeline (including connectivity)
  // without having to worry about the order. Here is how to do it:
  // @verbatim
  // * Create all proxies
  // * Set all property values - make sure that input properties
  //      do not auto update by calling 
  //      vtkSMInputProperty::SetInputsUpdateImmediately(0); 
  // * Call UpdateSelfAndAllInputs() on either all proxies or
  //   one that depends on all others (usually one or more DisplayWindows)
  // * If necessary vtkSMInputProperty::SetInputsUpdateImmediately(1); 
  // @endverbatim
  virtual void UpdateSelfAndAllInputs();

  // Description:
  // Returns the type of object managed by the proxy.
  vtkGetStringMacro(VTKClassName);

  // Description:
  // Overloaded to break the reference loop caused by the fact that
  // proxies store their own ClientServer ids.
  virtual void UnRegister(vtkObjectBase* obj);

  // Description:
  // Returns the id of a server object.
  vtkClientServerID GetID(unsigned int idx);

  // Description:
  // Returns the number of server ids (same as the number of server objects
  // if CreateVTKObjects() has already been called)
  unsigned int GetNumberOfIDs();

  // Description:
  // Returns a new (initialized) iterator of the properties.
  vtkSMPropertyIterator* NewPropertyIterator();

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
  // Assigned by the XML parser. The name assigned in the XML
  // configuration. Can be used to figure out the origin of the
  // proxy.
  vtkGetStringMacro(XMLName);

  // Description:
  // Updates all property informations by calling UpdateInformation()
  // and populating the values. It also calls UpdateDependentDomains()
  // on all properties to make sure that domains that depend on the
  // information are updated.
  virtual void UpdateInformation();
  
protected:
  vtkSMProxy();
  ~vtkSMProxy();

//BTX
  // These classes have been declared as friends to minimize the
  // public interface exposed by vtkSMProxy. Each of these classes
  // use a small subset of protected methods. This should be kept
  // as such.
  friend class vtkSMProperty;
  friend class vtkSMProxyManager;
  friend class vtkSMInputProperty;
  friend class vtkSMProxyProperty;
  friend class vtkSMDisplayerProxy;
  friend class vtkSMDisplayWindowProxy;
  friend class vtkSMProxyObserver;
  friend class vtkSMSourceProxy;
  friend class vtkSMPropertyIterator;
  friend class vtkSMPartDisplay;
  friend class vtkSMLODPartDisplay;
  friend class vtkSMCompositePartDisplay;
  friend class vtkSMPickDisplay;
  friend class vtkSMPlotDisplay;
  friend class vtkSMCubeAxesDisplay;
  friend class vtkPVPointSourceWidget;
  friend class vtkPVLineSourceWidget;
  friend class vtkPVImplicitPlaneWidget;
  friend class vtkPVSphereWidget;
  friend class vtkPVBoxWidget;
  friend class vtkSMBoxWidgetProxy;
  // Temporary
  friend class vtkPVSource;
//ETX

  // Description:
  // the type of object created by the proxy.
  // This is used only when creating the server objects. Once the server
  // object(s) have been created, changing this has no effect.
  vtkSetStringMacro(VTKClassName);

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
  // Given the number of objects (numObjects), class name (VTKClassName)
  // and server ids ( this->GetServerIDs()), this methods instantiates
  // the objects on the server(s)
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // UnRegister all managed objects. This also resets the ID list.
  // However, it does not remove the properties.
  void UnRegisterVTKObjects();

  // IDs are used to access server objects using the stream-based wrappers.
  // The following methods manage the IDs of objects maintained by the proxy.
  // Note that the IDs are assigned by the proxy at creation time. They
  // can not be set.

  // Description:
  // Add an ID to be managed by the proxy. In this case, the proxy
  // takes control of the reference (it unassigns the ID in destructor).
  // One easy of creating an empty proxy and assigning IDs to it is:
  // proxy->SetVTKClassName("foobar");
  // proxy->CreateVTKObjects(0);
  // proxy->SetID(0, id1);
  // proxy->SetID(1, id2);
  void SetID(unsigned int idx, vtkClientServerID id);

  // Server IDs determine on which server(s) the VTK objects are
  // instantiated. Use the following methods to set/get the server
  // IDs. Server IDs have to be set before the object is created.
  // Changing them after creation has no effect.
  // See vtkProcessModule.h for a list of all server types.
  // To add a server, OR it's value with the servers ivar.

  // Description:
  // Return the servers.
  vtkTypeUInt32 GetServers();

  // Description:
  // Set server ids on self and sub-proxies.
  void SetServers(vtkTypeUInt32 servers);

  // Description:
  // Set server ids on self
  void SetServersSelf(vtkTypeUInt32 servers);

//BTX
  // This is a convenience method that pushes the value of one property
  // to one server alone. This is most commonly used by sub-classes
  // to make calls on the server manager through the stream interface.
  // This method does not change the modified flag of the property.
  // If possible, use UpdateVTKObjects() instead of this.
  void PushProperty(const char* name, 
                    vtkClientServerID id, 
                    vtkTypeUInt32 servers);
//ETX

  // Note on property modified flags:
  // The modified flag of each property associated with a proxy is
  // stored in the proxy object instead of in the property itself.
  // Here is a brief explanation of how modified flags are used:
  // 1. When a property is modified, the modified flag is set
  // 2. In UpdateVTKObjects(), the proxy visits all properties and
  //    calls AppendCommandToStream() on each modified property.
  //    It also resets the modified flag.
  // The reason why the modified flag is stored in the proxy instead
  // of property is in item 2 above. If multiple proxies were sharing the same
  // property, the first one would reset the modified flag in
  // UpdateVTKObjects() and then others would not call AppendCommandToStream()
  // in their turn. Therefore, each proxy has to keep track of all
  // properties it updated.
  // This is done by adding observers to the properties. When a property
  // is modified, it invokes all observers and the observers set the
  // appropriate flags in the proxies. 


  // Description:
  // Cleanup code. Remove all observers from all properties assigned to
  // this proxy.  Called before deleting properties/
  void RemoveAllObservers();

  // Description:
  // Changes the modified flag of a property. Used by the observers
  void SetPropertyModifiedFlag(const char* name, int flag);

  // Description:
  // Add a property to either self (subProxyName = 0) or a sub-proxy.
  // IMPORTANT: If subProxyName = 0, AddProperty() checks for a
  // proxy with the given name in self and all sub-proxies, if one
  // exists, it replaces it. In this special case, it is possible for
  // the property to be added to a sub-proxy as opposed to self.
  void AddProperty(const char* subProxyName,
                   const char* name, 
                   vtkSMProperty* prop);

  // Description:
  // Remove a property from the list.
  void RemoveProperty(const char* name);

  // Description:
  // Add a property to self.
  void AddPropertyToSelf(const char* name, vtkSMProperty* prop);

  // Description:
  // Add a sub-proxy.
  void AddSubProxy(const char* name, vtkSMProxy* proxy);

  // Description:
  // Remove a sub-proxy.
  void RemoveSubProxy(const char* name);

  // Description:
  // Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
  vtkSMProxy* GetSubProxy(const char* name);

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
  // Calls MarkConsumersAsModified() on all consumers. Sub-classes
  // should add their functionality and call this.
  virtual void MarkConsumersAsModified();

  // Description:
  // Creates a new proxy and initializes it by calling ReadXMLAttributes()
  // with the right XML element.
  vtkSMProperty* NewProperty(const char* name);

  // Description:
  // Return a property of the given name from self or one of
  // the sub-proxies. If selfOnly is set, the sub-proxies are
  // not checked.
  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  // Description:
  // Read attributes from an XML element.
  int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  char* VTKClassName;
  char* XMLGroup;
  char* XMLName;
  int ObjectsCreated;
  vtkTypeUInt32 Servers;
  int DoNotModifyProperty;

  vtkClientServerID SelfID;

  void SetXMLElement(vtkPVXMLElement* element);
  vtkPVXMLElement* XMLElement;

  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

private:
  vtkSMProxyInternals* Internals;

  vtkSMProxy(const vtkSMProxy&); // Not implemented
  void operator=(const vtkSMProxy&); // Not implemented
};

#endif
