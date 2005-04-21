/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProperty - superclass for all SM properties
// .SECTION Description
// Each instance of vtkSMProperty or a sub-class represents a method 
// and associated arguments (if any) of a  a vtk object stored on one 
// or more client manager or server nodes. It may have a state and can push 
// this state to the vtk object it refers to. vtkSMPropery only supports
// methods with no arguments. Sub-classes support methods with different
// arguments types and numbers.
// A property can also be marked as an "information" property in which case
// it's values are obtained from the server with the UpdateInformation()
// call. This call is forwarded to an information helper. The type of
// the information helper used is specified in the XML file.
// Each property can have one or more sub-properties. The sub-properties
// can be accessed using an iterator. It is possible to create composite
// properties of any depth this way.
// Each property can have one or more domains. A domain represents a
// set of acceptable values the property can have. An Attempt to set a
// value outside the domain will fail. If more than one domain is specified,
// the actual domain is the intersection of all domains.
// 
// A property can be marked "animateable". "animateable" attribute can have 
// value {0, 1, 2}. 
// 0 :-- property is not animateable at all. 
// 1 :-- property is shown as animateable on the key frame animation interface
//       in the non-advanced mode.
// 2 :-- property is animateable but only shown in the advanced mode.
// Properties that are not vector properties or that don't have domain or that 
// are information_only properties
// can never be animated irrespective of the "animateable" attribute's value.
// All vector properties (vtkSMIntVectorProperty, vtkSMDoubleVectorPropery,
// vtkSMStringVectorProperty, vtkSMIdTypeVectorProperty) by default have
// have animateable="2".
//
// A property can be marked "saveable". "saveable" attribute can have 
// value {0, 1}.
// 0 :-- property is not saveable. Hence, when saving ServerManager state
//       or saving batch script, the property's value is not saved.
// 1 :-- property is saveable. Hence, it gets saved when saving SM state
//       or batch script. 
// An instance of vtkSMProperty is never savable. All other concrete subclasses
// are by default savable (i.e. vtkSMProxyProperty and subclasses and
// vtkSMVectorProperty subclasses).
// .SECTION See Also
// vtkSMProxyProperty vtkSMInputProperty vtkSMVectorProperty
// vtkSMDoubleVectorPropery vtkSMIntVectorPropery vtkSMStringVectorProperty
// vtkSMDomain vtkSMInformationHelper

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSMDomain;
class vtkSMDomainIterator;
class vtkSMInformationHelper;
class vtkSMProxy;
class vtkSMXMLParser;
//BTX
struct vtkSMPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProperty : public vtkSMObject
{
public:
  static vtkSMProperty* New();
  vtkTypeRevisionMacro(vtkSMProperty, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The command name used to set the value on the server object.
  // For example: SetThetaResolution
  vtkSetStringMacro(Command);
  vtkGetStringMacro(Command);

  // Description:
  // If ImmediateUpdate is true, the value of the property will
  // be pushed to the server as soon as it is modified. Properties
  // that do not have values can be pushed by calling Modified().
  vtkSetMacro(ImmediateUpdate, int);
  vtkGetMacro(ImmediateUpdate, int);

  // Description:
  // Advanced. If UpdateSelf is true, the property will be pushed
  // by calling the method (Command) on the proxy instead of the
  // VTK object. This is commonly used to implement more complicated
  // functionality than can be obtained by calling a method on all
  // server objects.
  vtkSetMacro(UpdateSelf, int);
  vtkGetMacro(UpdateSelf, int);

  // Description:
  // Returns a sub-property with the given name. If the sub-property
  // does not exist, NULL is returned.
  vtkSMProperty* GetSubProperty(const char* name);

  // Description:
  // Returns true if all values are in all domains, false otherwise.
  // The domains will check the unchecked values (SetUncheckedXXX()) 
  // instead of the actual values.
  int IsInDomains();

  // Description:
  // Overloaded to break the reference loop caused by the 
  // internal domain iterator.
  virtual void UnRegister(vtkObjectBase* obj);

  // Description:
  // Creates, initializes and returns a new domain iterator. The user 
  // has to delete the iterator.
  vtkSMDomainIterator* NewDomainIterator();

  // Description:
  // Returns a domain give a name. 
  vtkSMDomain* GetDomain(const char* name);

  // Description:
  // Returns the number of domains this property has. This can be 
  // used to specify a valid index for GetDomain(index).
  unsigned int GetNumberOfDomains();

  // Description:
  // Calls Update() on all domains contained by the property
  // as well as all dependant domains. This is usually called
  // after SetUncheckedXXX() to tell all dependant domains to
  // update themselves according to the new value.
  // Note that when calling Update() on domains contained by
  // this property, a NULL is passed as the argument. This is
  // because the domain does not really "depend" on the property.
  // When calling Update() on dependent domains, the property
  // passes itself as the argument.
  void UpdateDependentDomains();

  // Description:
  // Static boolean used to determine whether domain checking should
  // be performed when setting values. On by default.
  static int GetCheckDomains();
  static void SetCheckDomains(int check);

  // Description:
  // The name assigned by the xml parser. Used to get the property
  // from a proxy.
  vtkGetStringMacro(XMLName);

  // Description:
  // Is InformationOnly is set to true, this property is used to
  // get information from server instead of setting values.
  vtkGetMacro(InformationOnly, int);

  // Description:
  // Get the associated information property. This allows applications
  // to have access to both the in and out properties. The information
  // property has to be specified in the xml configuration file.
  vtkGetObjectMacro(InformationProperty, vtkSMProperty);

  // Description:
  // ControllerProxy is pointer to the proxy whose property
  // (ControllerProperty) is mapped to the current property. This is useful
  // for 3DWidgets. The properties of the implicit function proxy
  // controlled by the 3DWidget will have these set to the corresponing
  // property of the 3DWidget. Thus, providing hints about which implicit
  // function property is controlled by which 3DWidget and what property.
  // This goes mostly unnoticed in ParaView, but useful for ARL.  If these
  // are set, then they are saved in the XML during SaveState as a element
  // <ControllerProperty name="propertyname" />. ARL notices such elements
  // and creates a property value dependency among the controlee and the
  // controller property.
  void SetControllerProxy(vtkSMProxy* proxy);
  void SetControllerProperty(vtkSMProperty* property);

  // Description:
  // Get/Set if the property is animateable. Non-animateable properties are shown in the
  // GUI only in advanced mode.
  vtkSetMacro(Animateable, int);
  vtkGetMacro(Animateable, int);

  // Description:
  // Get/Set if the property is saveable. A non-saveable property should not
  // be saved in state or batch script.
  vtkSetMacro(Saveable, int);
  vtkGetMacro(Saveable, int);

  // Description: 
  // Copy all property values.
  virtual void DeepCopy(vtkSMProperty* src);

protected:
  vtkSMProperty();
  ~vtkSMProperty();

  //BTX
  friend class vtkSMProxyManager;
  friend class vtkSMProxy;
  friend class vtkSMSubPropertyIterator;
  friend class vtkSMDomainIterator;
  friend class vtkSMSourceProxy;
  friend class vtkSMDomain;
  friend class vtkPVPointSourceWidget;

  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );

  // Description:
  // If this is an information property (InformationOnly is true),
  // this method fills the vector with the values obtained from
  // the server. This work is forwarded to the information helper.
  virtual void UpdateInformation(int serverids, vtkClientServerID objectId);
  //ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

  // Description:
  // Update all proxies referred by this property (if any). Overwritten
  // by vtkSMProxyProperty and sub-classes.
  virtual void UpdateAllInputs() {};

  // Description:
  // Properties can have one or more domains. These are assigned by
  // the proxy manager and can be used to obtain information other
  // than given by the type of the propery and its values.
  void AddDomain(const char* name, vtkSMDomain* dom);

  // Description:
  // The name assigned by the xml parser. Used to get the property
  // from a proxy.
  vtkSetStringMacro(XMLName);

  // Description:
  // Add a sub-property with the given name.
  void AddSubProperty(const char* name, vtkSMProperty* proxy);

  // Description:
  // Remove the named sub-property.
  void RemoveSubProperty(const char* name);

  // Description:
  // Internal. Used during XML parsing to get a property with
  // given name. Used by the domains when setting required properties.
  vtkSMProperty* NewProperty(const char* name);

  // Description:
  // Internal. Used by the domains that require this property. They
  // add themselves as dependents.
  void AddDependent(vtkSMDomain* dom);

  // Description:
  // Removes all dependents.
  void RemoveAllDependents();

  // Description:
  // Save the state in XML.
  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);

  // Description:
  // Set from the XML file, information helpers fill in the property
  // values with information obtained from server.
  void SetInformationHelper(vtkSMInformationHelper* helper);

  char* Command;

  vtkSMPropertyInternals* PInternals;

  int ImmediateUpdate;
  int UpdateSelf;
  int Animateable;
  int Saveable;

  char* XMLName;

  vtkSMDomainIterator* DomainIterator;

  static int CheckDomains;

  // ControllerProxy is pointer to the proxy whose property (ControllerProperty) is
  // mapped to the current property. This is useful for 3DWidgets. The properties of
  // the implicit function proxy controlled by the 3DWidget will have these set to the 
  // corresponing property of the 3DWidget. Thus, providing hints about which implicit function
  // property is controlled by which 3DWidget and what property.
  // This goes mostly unnoticed in ParaView, but useful for ARL. 
  // If these are set, then they are saved in the XML during SaveState as a
  // element <ControllerProperty name="propertyname" />. ARL notices such elements and
  // creates a property value dependency among the controlee and the controller property.
  vtkSMProxy* ControllerProxy;
  vtkSMProperty* ControllerProperty;
  
  // Set during xml parsing only. Do not use outside ReadXMLAttributes().
  vtkSMProxy* Proxy;
  void SetProxy(vtkSMProxy* proxy);

  vtkSetMacro(InformationOnly, int);
  int InformationOnly;

  vtkSMInformationHelper* InformationHelper;

  void SetInformationProperty(vtkSMProperty* ip);
  vtkSMProperty* InformationProperty;

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented
};

#endif
