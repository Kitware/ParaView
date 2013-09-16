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
// and associated arguments (if any) of a VTK object stored on one 
// or more client manager or server nodes. It may have a state and can push 
// this state to the vtk object it refers to. vtkSMProperty only supports
// methods with no arguments. Sub-classes support methods with different
// arguments types and numbers.
//
// Property is typically meant for pushing its values to athe VTK object.
// However, a property may be marked as an InformationOnly property
// in which case its values are obtained from the server with the
// UpdateInformation() call.
//
// Each non-information property can have one or more domains. A domain represents a
// set of acceptable values the property can have. Domains provide applications
// mechanisms to extract semantic inform a property.
//
// A property has two kinds of values: regular (or checked) values and unchecked
// values. Regular values are the ones that are pushed to the VTK object when
// the property is updated. These are the ones that get saved in state, etc.
// Unchecked values are provided so that domains can update their conditions
// without having the change the property's value e.g. if the domain range for the
// IsoContour property changes based on the value of the ArrayName property
// which selects the array to contour with, the can set the unchecked value on
// the ArrayName property to each of the available arrays to determine what the
// domain would be without having to modify the property and update its VTK
// object. If a property has no unchecked-values explicitly set, then the "Get"
// methods that access the unchecked-values should simply return the checked
// values. If the checked values are changed, the unchecked values are reset to
// match the checked values.
//
// A property fires the following events:
//
// \li \b vtkCommand::ModifiedEvent : fired when property's value(s) is(are)
//        modified. This must be fired only when values are really changed, not
//        just the "set" methods are called. This event must be fired no matter
//        how the property's values are changed.
//
// \li \b vtkCommand::UncheckedPropertyModifiedEvent : fired when the property's
//        unchecked-value(s) is(are) changed. Note that when a property's
//        checked values change, it's unchecked values are reset to match the
//        checked values, so technically,
//        vtkCommand::UncheckedPropertyModifiedEvent must be fired every time
//        vtkCommand::ModifiedEvent is fired.
//
// Properties are typically constructed from ServerManager XML configuration
// files. Attributes available on a Property XML are as follows:
//
// \li \b name: \c string: This is the name for the property. This typically
//        ends up being the name used by the Proxy to refer to this property. It
//        must be unique for all properties on a Proxy.
//
// \li \b label: \c string:This is the user-friendly label. Ideally, the label
//        should be same as the name, however traditionally that hasn't been the
//        case.
//
// \li \b command: \c string: This is the name of the method to call on the VTK
//        update for to property.
//
// \li \b repeatable or \b repeat_command: \c{0, 1}: This used to indicate that
//        the command can be called repeatedly to update the VTK object. e.g.
//        for multiple inputs, one must call AddInput(..) repeatedly. It also
//        implies that the number of elements/items in the property can change.
//
// \li \b information_only: \c{0, 1}: When set, it implies that this property
//        is used to obtain values from the VTK object, rather than the default
//        which is set values on the VTK object.
//
// \li \b information_property: \c string: Value is the name of the property on the
//        proxy to which this property belongs that can is information_only
//        property corresponding to this. This is useful when the variable that
//        this property sets can be changed by other means besides this property
//        e.g. through interaction. Applications can use this information to
//        update the value of this property to reflect the VTK-side state.
//
// \li \b immediate_update: \c{0, 1}: When set, the Proxy will attempt to push
//        the value for this property to the VTK object as soon as the property
//        is changed. It is no longer common and should be avoided. It may be
//        deprecated in near future.
//
// \li \b state_ignored: \c{0, 1}: When set, changes to this property are not
//        captured in undo-redo stacks. Unlike is_internal, the value for this
//        property is saved in state files.
//
// \li \b ignore_synchronization: \c{0, 1}: When set, changes to this property
//        are not synchronized among client-processes in collaborative mode.
//
// \li \b is_internal: \c{0, 1}: When set, the property is treated as internal
//        which implies that it will not be shown in the UI; it value will not
//        be pushed when the proxy is created, nor saved in state files or
//        undo-redo stacks.
//
// \li \b animateable: \c{0, 1}: When set, the property is considered as
//        animatable which the UI can use to build the animation interface.
//
// \li \b panel_visibility: \c{default, advanced, never}: Indicates to the UI
//        that the widget corresponding to this property should be shown in the
//        default or advanced mode, or never at all.
//
// \li \b panel_widget: \c string: provides a hint to the UI to determine which
//        what widget to create to edit this property.

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
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
//BTX
struct vtkSMPropertyInternals;
//ETX

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProperty : public vtkSMObject
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
  // @deprecated This method is no longer needed. Dependent domains are now
  // automatically updated when a property fires
  // vtkCommand::UncheckedPropertyModifiedEvent. The implementation has been
  // changed to do nothing and the method will be removed in future releases.
  VTK_LEGACY(void UpdateDependentDomains());

  // Description:
  // Is InformationOnly is set to true, this property is used to
  // get information from server instead of setting values.
  vtkGetMacro(InformationOnly, int);

  // Description:
  // If IgnoreSynchronization is set to true, this property is used to
  // prevent that property from beeing updated when changed remotely by another
  // collaborative client.
  vtkGetMacro(IgnoreSynchronization, int);

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
  // Sets the panel visibility for the property. The value can be
  // one of:
  //   * "default": Show the property by default.
  //   * "advanced": Only show the property in the advanced view.
  //   * "never": Never show the property on the panel.
  //
  // By default, all properties have "default" visibility.
  vtkSetStringMacro(PanelVisibility)

  // Description:
  // Returns the panel visibility for the property.
  vtkGetStringMacro(PanelVisibility)

  // Description:
  // Sets the panel visibility to default if the current
  // representation type matches \p representation.
  vtkSetStringMacro(PanelVisibilityDefaultForRepresentation)

  // Description:
  // Returns which representation type the property will be shown by
  // default for.
  vtkGetStringMacro(PanelVisibilityDefaultForRepresentation)

  // Description:
  // Sets the name of the custom panel widget to use for the property.
  vtkSetStringMacro(PanelWidget)

  // Description:
  // Returns name of the panel widget for the property.
  vtkGetStringMacro(PanelWidget)

  // Description:
  // Copy all property values. This will copy both checked and unchecked values,
  // if applicable.
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

  // Description:
  // Returns true if the property's value is different from the default value.
  // This is used as a hint by the state saving code to determine if the value
  // should be written to the file or if the defaults are sufficient.
  virtual bool IsValueDefault() { return false; }

//BTX
protected:
  vtkSMProperty();
  ~vtkSMProperty();

  friend class vtkSMSessionProxyManager;
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
  virtual void ReadFrom( const vtkSMMessage*, int vtkNotUsed(message_offset),
                         vtkSMProxyLocator*) {};

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
  // Calls Update() on all domains contained by the property
  // as well as all dependant domains. This is autimatically called
  // after SetUncheckedXXX() to tell all dependant domains to
  // update themselves according to the new value.
  // Note that when calling Update() on domains contained by
  // this property, a NULL is passed as the argument. This is
  // because the domain does not really "depend" on the property.
  // When calling Update() on dependent domains, the property
  // passes itself as the argument.
  void UpdateDomains();

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

  char* PanelVisibility;
  char* PanelVisibilityDefaultForRepresentation;
  char* PanelWidget;

  vtkSMDomainIterator* DomainIterator;

  vtkSetMacro(InformationOnly, int);
  int InformationOnly;

  vtkSetMacro(IgnoreSynchronization, int);
  int IgnoreSynchronization;

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
