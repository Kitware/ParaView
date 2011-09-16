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
// A property can be marked "is_internal". "internal" attribute can have 
// value {0, 1}.
// 0 :-- property is not internal.
//
// 1 :-- property is internal. Hence, it is not saved when saving SM state
//       or batch script. 
// An instance of vtkSMProperty is always internal. All other concrete subclasses
// are by default not internal (i.e. vtkSMProxyProperty and subclasses and
// vtkSMVectorProperty subclasses).
// .SECTION See Also
// vtkSMProxyProperty vtkSMInputProperty vtkSMVectorProperty
// vtkSMDoubleVectorPropery vtkSMIntVectorPropery vtkSMStringVectorProperty
// vtkSMDomain vtkSMInformationHelper

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkSMObject.h"
#include "vtkSMMessageMinimal.h" // needed for vtkSMMessage
#include "vtkWeakPointer.h" // needed for vtkweakPointer

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSMDocumentation;
class vtkSMDomain;
class vtkSMDomainIterator;
class vtkSMInformationHelper;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMStateLocator;
//BTX
struct vtkSMPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProperty : public vtkSMObject
{
public:
  static vtkSMProperty* New();
  vtkTypeMacro(vtkSMProperty, vtkSMObject);
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
  // Returns true if all values are in all domains, false otherwise.
  // The domains will check the unchecked values (SetUncheckedXXX()) 
  // instead of the actual values.
  int IsInDomains();

  // Description:
  // Overload of IsInDomains() that provides a mechanism to return the first
  // domain that fails the check. \c domain is set to NULL when all domain
  // checks pass.
  int IsInDomains(vtkSMDomain** domain);

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
  // Returns the first domain which is of the specified type.
  vtkSMDomain* FindDomain(const char* classname);

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
  // Is InformationOnly is set to true, this property is used to
  // get information from server instead of setting values.
  vtkGetMacro(InformationOnly, int);

  // Description:
  // Get the associated information property. This allows applications
  // to have access to both the in and out properties. The information
  // property has to be specified in the xml configuration file.
  vtkGetObjectMacro(InformationProperty, vtkSMProperty);

  // Description:
  // Properties can have one or more domains. These are assigned by
  // the proxy manager and can be used to obtain information other
  // than given by the type of the propery and its values.
  void AddDomain(const char* name, vtkSMDomain* dom);

  // Description: 
  // Get/Set if the property is animateable. Non-animateable
  // properties are shown in the GUI only in advanced mode.
  vtkSetMacro(Animateable, int);
  vtkGetMacro(Animateable, int);

  // Description:
  // Get/Set if the property is internal to server manager.
  // Internal properties are not saved in state and should not be
  // displayed in the user interface.
  vtkSetMacro(IsInternal, int);
  vtkGetMacro(IsInternal, int);

  // Description:
  // Copy all property values.
  virtual void Copy(vtkSMProperty* src);

  // Description:
  // Returns the documentation for this proxy. The return value
  // may be NULL if no documentation is defined in the XML
  // for this property.
  vtkGetObjectMacro(Documentation, vtkSMDocumentation);

  // Description:
  // Iterates over all domains and calls SetDefaultValues() on each
  // until one of then returns 1, implying that it updated
  // the property value. This is used to reset the property
  // to its default value. Currently default values that depend
  // on domain are reset. This method can also be called
  // to reset the property value to the default specified
  // in the configuration XML. If none of the domains
  // updates the property value, then some property subclassess
  // (viz. IntVectorProperty, DoubleVectorProperty and IdTypeVectorProperty)
  // update the current value to that specified in the configuration XML.
  void ResetToDefault();

  // Description:
  // The label assigned by the xml parser.
  vtkGetStringMacro(XMLLabel);

  // Description:
  // If repeatable, a property can have 1 or more values of the same kind.
  // This ivar is configured when the xml file is read and is mainly useful
  // for information (for example from python).
  vtkGetMacro(Repeatable, int);

  // Description:
  // The server manager configuration XML may define <Hints /> element for
  // a property. Hints are metadata associated with the property. The
  // Server Manager does not (and should not) interpret the hints. Hints
  // provide a mechanism to add GUI pertinant information to the server
  // manager XML.  Returns the XML element for the hints associated with
  // this property, if any, otherwise returns NULL.
  vtkGetObjectMacro(Hints, vtkPVXMLElement);

  // Description:
  // Overridden to support blocking of modified events.
  virtual void Modified()
    {
    if (this->BlockModifiedEvents)
      {
      this->PendingModifiedEvents = true;
      }
    else
      {
      this->Superclass::Modified();
      this->PendingModifiedEvents = false;
      }
    }

  // Description:
  // Get the proxy to which this property belongs. Note that is this property is
  // belong to a sub-proxy of a proxy, the returned value will indeed be that
  // sub-proxy (and not the outer container proxy).
  vtkSMProxy* GetParent();

  // Flag used to ignore property when building Proxy state for Undo/Redo state.
  // The default value is false.
  virtual bool IsStateIgnored() { return this->StateIgnored; }

//BTX
protected:
  vtkSMProperty();
  ~vtkSMProperty();

  friend class vtkSMProxyManager;
  friend class vtkSMProxy;
  friend class vtkSMSubPropertyIterator;
  friend class vtkSMDomainIterator;
  friend class vtkSMSourceProxy;
  friend class vtkSMDomain;
  friend class vtkSMPropertyModificationUndoElement;

  // Description:
  // Let the property write its content into the stream
  virtual void WriteTo(vtkSMMessage* msg);

  // Description:
  // Let the property read and set its content from the stream
  virtual void ReadFrom(const vtkSMMessage*, int vtkNotUsed(message_offset)) {};

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
  // The name assigned by the xml parser. Used to get the property
  // from a proxy. Note that the name used to obtain a property
  // that is on a subproxy may be different from the XMLName of the property,
  // see the note on ExposedProperties for vtkSMProxy.
  vtkSetStringMacro(XMLName);
  
  // Description:
  // The name assigned by the xml parser. Used to get the property
  // from a proxy. Note that the name used to obtain a property
  // that is on a subproxy may be different from the XMLName of the property,
  // see the note on ExposedProperties for vtkSMProxy.
  vtkGetStringMacro(XMLName);

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
  // Save the property state in XML.
  // This method create the property definition and rely on SaveStateValues
  // to fill this definition with the concrete property values
  virtual void SaveState(vtkPVXMLElement* parent, const char* property_name,
                         const char* uid, int saveDomains=1);
  // Description:
  // This method must be overiden by concrete class in order to save the real
  // property data
  virtual void SaveStateValues(vtkPVXMLElement* propertyElement);

  // Description:
  // Save property domain
  virtual void SaveDomainState(vtkPVXMLElement* propertyElement, const char* uid);

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader);

  void SetHints(vtkPVXMLElement* hints);
  vtkPVXMLElement* Hints;

  char* Command;

  vtkSMPropertyInternals* PInternals;

  int ImmediateUpdate;
  int Animateable;
  int IsInternal;

  char* XMLName;
  char* XMLLabel;
  vtkSetStringMacro(XMLLabel);

  vtkSMDomainIterator* DomainIterator;

  vtkSetMacro(InformationOnly, int);
  int InformationOnly;

  vtkSMInformationHelper* InformationHelper;

  void SetInformationProperty(vtkSMProperty* ip);
  vtkSMProperty* InformationProperty;

  vtkSMDocumentation* Documentation;
  void SetDocumentation(vtkSMDocumentation*);

  // Subclass may override this if ResetToDefault can reset to default
  // value specified in the configuration file.
  virtual void ResetToDefaultInternal() {};

  int Repeatable;

  // Description:
  // Block/unblock modified events, returns the current state of the block flag.
  bool SetBlockModifiedEvents(bool block)
    {
    bool prev = this->BlockModifiedEvents;
    this->BlockModifiedEvents = block;
    return prev;
    }

  // Description:
  // Returns if any modified evetns are pending.
  // This gets cleared when Modified() is called.
  vtkGetMacro(PendingModifiedEvents, bool);

  // Proxy is not reference-counted to avoid reference loops.
  void SetParent(vtkSMProxy* proxy);

  vtkWeakPointer<vtkSMProxy> Proxy;

  // Flag used to ignore property when building Proxy state for Undo/Redo state.
  // The default value is false.
  bool StateIgnored;
  vtkSetMacro(StateIgnored, bool);
  vtkBooleanMacro(StateIgnored, bool);

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented

  // Description:
  // Given the string, this method will create and set a well-formated
  // string as XMLLabel.
  void CreatePrettyLabel(const char* name);

  bool PendingModifiedEvents;
  bool BlockModifiedEvents;
//ETX
};

#endif
