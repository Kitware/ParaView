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
// Each property can have one or more sub-properties. The sub-properties
// can be accessed using an iterator. It is possible to create composite
// properties of any depth this way.
// Each property can have one or more domains. A domain represents a
// set of acceptable values the property can have. An Attempt to set a
// value outside the domain will fail. If more than one domain is specified,
// the actual domain is the intersection of all domains.
// .SECTION See Also
// vtkSMProxyProperty vtkSMInputProperty vtkSMVectorProperty
// vtkSMDoubleVectorPropery vtkSMIntVectorPropery vtkSMStringVectorProperty

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSMDomain;
class vtkSMDomainIterator;
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
  // Advanced. When IsReadOnly is true, the value(s) of the property
  // cannot be changed.
  vtkGetMacro(IsReadOnly, int);

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
  // Calls Update() on all domains contained by the property
  // as well as all dependant domains. This is usually called
  // after SetUncheckedXXX() to tell all dependant domains to
  // update themselves according to the new value.
  void UpdateDependentDomains();

  // Description:
  // Static boolean used to determine whether domain checking should
  // be performed when setting values. On by default.
  static int GetCheckDomains();
  static void SetCheckDomains(int check);

  // Description:
  // Static boolean used to determine whether a property is marked
  // as modified when first created. Turn this on to get all the
  // default values specified in the XML in the first UpdateVTKObjects.
  // On by default.
  // Note: if this is set to on, the proxy and the server object(s)
  // will be out-of-sync until all property values are modified and
  // pushed.
  static int GetModifiedAtCreation();
  static void SetModifiedAtCreation(int check);

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

  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );

  // Description:
  virtual void UpdateInformation(vtkClientServerID) {};
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
  vtkGetStringMacro(XMLName);

  // Description:
  // Add a sub-property with the given name.
  void AddSubProperty(const char* name, vtkSMProperty* proxy);

  // Description:
  // Remove the named sub-property.
  void RemoveSubProperty(const char* name);

  // Description:
  // Advanced. When IsReadOnly is true, the value(s) of the property
  // cannot be changed.
  vtkSetMacro(IsReadOnly, int);

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
  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  char* Command;

  vtkSMPropertyInternals* PInternals;

  int ImmediateUpdate;
  int UpdateSelf;
  int IsReadOnly;

  char* XMLName;

  vtkSMDomainIterator* DomainIterator;

  static int CheckDomains;
  static int ModifiedAtCreation;

  // Set during xml parsing only. Do not use outside ReadXMLAttributes().
  vtkSMProxy* Proxy;
  void SetProxy(vtkSMProxy* proxy);

  vtkSetMacro(InformationOnly, int);
  vtkGetMacro(InformationOnly, int);
  int InformationOnly;

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented
};

#endif
