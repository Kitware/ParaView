/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMDomain
 * @brief   represents the possible values a property can have
 *
 * vtkSMDomain is an abstract class that describes the "domain" of a
 * a widget. A domain is a collection of possible values a property
 * can have.
 *
 * Each domain can depend on one or more properties to compute it's
 * values. This are called "required" properties and can be set in
 * the XML configuration file.
 *
 * Every time a domain changes it must fire a vtkCommand::DomainModifiedEvent.
 * Applications may decide to update the UI every-time the domain changes. As a
 * result, domains ideally should only fire that event when their values change
 * for real not just potentially changed.
 * @sa vtkSMDomain::DeferDomainModifiedEvents.
 *
 */

#ifndef vtkSMDomain_h
#define vtkSMDomain_h

#include "vtkClientServerID.h"              // needed for saving animation in batch script
#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMSessionObject.h"

class vtkPVDataInformation;
class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMProxyLocator;

struct vtkSMDomainInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMDomain : public vtkSMSessionObject
{
public:
  static vtkSMDomain* New();
  vtkTypeMacro(vtkSMDomain, vtkSMSessionObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return values for `IsInDomain` calls.
   */
  enum IsInDomainReturnCodes
  {
    NOT_APPLICABLE = -1,
    NOT_IN_DOMAIN = 0,
    IN_DOMAIN = 1,
  };

  /**
   * Is the (unchecked) value of the property in the domain? Overwritten by
   * sub-classes.
   *
   * Returned values as defined in `IsInDomainReturnCodes`. `NOT_APPLICABLE` is
   * returned if the domain is not applicable for the property values.
   * `NOT_IN_DOMAIN` implies that the value is not in domain while `IN_DOMAIN`
   * implies that the value is acceptable.
   *
   */
  virtual int IsInDomain(vtkSMProperty* vtkNotUsed(property)) { return IN_DOMAIN; }

  /**
   * Update self based on the "unchecked" values of all required
   * properties. Subclasses must override this method to update the domain based
   * on the requestingProperty (and/or other required properties).
   */
  virtual void Update(vtkSMProperty* requestingProperty);

  /**
   * Set the value of an element of a property from the animation editor.
   */
  virtual void SetAnimationValue(vtkSMProperty*, int vtkNotUsed(index), double vtkNotUsed(value)) {}

  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   * If \c use_unchecked_values is true, the property's unchecked values will be
   * changed by this method.
   * Returns 1 if the domain updated the property.
   * Default implementation does nothing.
   */
  virtual int SetDefaultValues(vtkSMProperty*, bool vtkNotUsed(use_unchecked_values)) { return 0; };

  //@{
  /**
   * Assigned by the XML parser. The name assigned in the XML
   * configuration. Can be used to figure out the origin of the
   * domain.
   */
  vtkGetStringMacro(XMLName);
  //@}

  //@{
  /**
   * When the IsOptional flag is set, IsInDomain() always returns true.
   * This is used by properties that use domains to provide information
   * (a suggestion to the gui for example) as opposed to restrict their
   * values.
   */
  vtkGetMacro(IsOptional, bool);
  //@}

  /**
   * Provides access to the vtkSMProperty on which this domain is hooked up.
   */
  vtkSMProperty* GetProperty();

  //@{
  /**
   * Helper methods to get vtkPVDataInformation from input proxy connected to the
   * required property with the given function and provided input index.
   */
  virtual vtkPVDataInformation* GetInputDataInformation(
    const char* function, unsigned int index = 0);
  virtual vtkPVDataInformation* GetInputSubsetDataInformation(
    unsigned int compositeIndex, const char* function, unsigned int index = 0);
  //@}

  /**
   * Helper method to get the number of input connections hence the number of available
   * vtkPVDataInformation
   * from input proxy connected to the required property with the given function.
   */
  virtual unsigned int GetNumberOfInputConnections(const char* function);

  /**
   * Returns a given required property of the given function.
   * Function is a string associated with the require property
   * in the XML file.
   */
  vtkSMProperty* GetRequiredProperty(const char* function);

protected:
  vtkSMDomain();
  ~vtkSMDomain() override;

  //@{
  /**
   * Add the header and creates a new vtkPVXMLElement for the
   * domain, fills it up with the common attributes. The newly
   * created element will also be added to the parent element as a child node.
   * Subclasses can override ChildSaveState() method to fill it up with
   * subclass specific values.
   */
  void SaveState(vtkPVXMLElement* parent, const char* uid);
  virtual void ChildSaveState(vtkPVXMLElement* domainElement);
  //@}

  /**
   * Load the state of the domain from the XML.
   * Subclasses generally have no need to load XML state. In fact, serialization
   * of state for domains, in general, is unnecessary for two reasons:
   * 1. domains whose values are defined in XML will be populated from the
   * configuration xml anyways.
   * 2. domains whose values depend on data (at runtime) will be repopulated
   * with data values when the XML state is loaded.
   * The only exception to this rule is vtkSMProxyListDomain (presently).
   * vtkSMProxyListDomain needs to try to restore proxies (with proper ids) for
   * the domain.
   */
  virtual int LoadState(
    vtkPVXMLElement* vtkNotUsed(domainElement), vtkSMProxyLocator* vtkNotUsed(loader))
  {
    return 1;
  }

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* elem);

  friend class vtkSMProperty;

  /**
   * Remove the given property from the required properties list.
   */
  void RemoveRequiredProperty(vtkSMProperty* prop);

  /**
   * Add a new required property to this domain.
   * Whenever the \c prop fires vtkCommand::UncheckedPropertyModifiedEvent,
   * vtkSMDomain::Update(prop) is called. Also whenever a vtkSMInputProperty is
   * added as a required property, vtkSMDomain::Update(prop) will also be called
   * the vtkCommand::UpdateDataEvent is fired by the proxies contained in that
   * required property.
   */
  void AddRequiredProperty(vtkSMProperty* prop, const char* function);

  //@{
  /**
   * When the IsOptional flag is set, IsInDomain() always returns true.
   * This is used by properties that use domains to provide information
   * (a suggestion to the gui for example) as opposed to restrict their
   * values.
   */
  vtkSetMacro(IsOptional, bool);
  //@}

  //@{
  /**
   * Assigned by the XML parser. The name assigned in the XML
   * configuration. Can be used to figure out the origin of the
   * domain.
   */
  vtkSetStringMacro(XMLName);
  //@}

  /**
   * Invokes DomainModifiedEvent. Note that this event *must* be fired after the
   * domain has changed (ideally, if and only if the domain has changed).
   */
  void DomainModified();
  void InvokeModified() { this->DomainModified(); }

  /**
   * Gets the number of required properties added.
   */
  unsigned int GetNumberOfRequiredProperties();

  /**
   * Set the domain's property. This is called by vtkSMProperty when the domain
   * is created.
   */
  void SetProperty(vtkSMProperty*);

  /**
   * @class vtkSMDomain::DeferDomainModifiedEvents
   * @brief helper to defer firing of vtkCommand::DomainModifiedEvent.
   *
   * When sub-classing vtkSMDomain, we need to ensure that the domain fires
   * vtkCommand::DomainModifiedEvent if and only if the domain has been
   * modified. Oftentimes we may have to defer domain modified events till all
   * modifications have been done. This helper class helps us to that.
   *
   * For example, in the following code, the DomainModifiedEvent will only be
   * fired once if `something_changed` or `something_else_changed` are true when
   * `defer` goes out of scope.
   *
   * @code{cpp}
   * void ...::Update(...)
   * {
   *    DeferDomainModifiedEvents defer(this);
   *
   *    if (something_changed)
   *        this->DomainModified();
   *    if (something_else_changed)
   *        this->DomainModified();
   * }
   * @endcode
   *
   */
  class VTKREMOTINGSERVERMANAGER_EXPORT DeferDomainModifiedEvents
  {
    vtkSMDomain* Self;

  public:
    DeferDomainModifiedEvents(vtkSMDomain* self);
    ~DeferDomainModifiedEvents();
  };

  char* XMLName;
  bool IsOptional;
  vtkSMDomainInternals* Internals;

private:
  vtkSMDomain(const vtkSMDomain&) = delete;
  void operator=(const vtkSMDomain&) = delete;

  friend class DeferDomainModifiedEvents;
  unsigned int DeferDomainModifiedEventsCount;
  bool PendingDomainModifiedEvents;
};

#endif
