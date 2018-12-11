/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyProperty.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyProperty
 * @brief   property representing pointer(s) to vtkObject(s)
 *
 * vtkSMProxyProperty is a concrete sub-class of vtkSMProperty representing
 * pointer(s) to vtkObject(s) (through vtkSMProxy).
 *
 * vtkSMProperty::UpdateDomains() is called by vtkSMProperty itself whenever
 * its unchecked values are modified. In case of proxy-properties, the dependent
 * domains typically tend to depend on the data information provided by the
 * source-proxies added to the property. Thus, to ensure that the domains get
 * updated if the data information changes, vtkSMProxyProperty ensures that
 * vtkSMProperty::UpdateDomains() is called whenever any of the added proxies
 * fires the vtkCommand::UpdateDataEvent (which is fired whenever the pipeline
 * us updated through the ServerManager indicating that the data-information
 * last used may have been invalidated).
 *
 * Besides the standard set of attributes, the following XML attributes are
 * supported:
 * \li command : identifies the method to call on the VTK object e.g.
 * AddRepresentation.
 * \li clean_command : if present, called once before invoking the method
 * specified by \c command every time the property value is pushed e.g.
 * RemoveAllRepresentations. If property
 * can take multiple values then the \c command is called for for each of the
 * values after the clean command for every push.
 * \li remove_command : an alternative to clean_command where instead of
 * resetting and adding all the values for every push, this simply calls the
 * specified method to remove the vtk-objects no longer referred to e.g.
 * RemoveRepresentation.
 * \li argument_type : identifies the type for value passed to the method on the
 * VTK object. Accepted values are "VTK", "SMProxy" or "SIProxy". Default is
 * VTK.
 * \li null_on_empty : if set to 1, whenever the property's value changes to
 * empty i.e. it contains no proxies, the command is called on the VTK object
 * with NULL argument useful when there's no clean_command that can be called on
 * the VTK object to unset the property e.g. SetLookupTable(NULL).
 * \li skip_dependency: obsolete and no longer supported. This was intended for
 * vtkSMRepresentationProxy to distinguish between proxy connections that
 * invalidate representation pipeline (e.g. input) and those that don't (e.g.
 * LookupTable). vtkSMRepresentationProxy now handles this automatically.
 * @sa
 * vtkSMProperty
 */

#ifndef vtkSMProxyProperty_h
#define vtkSMProxyProperty_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMProperty.h"

class vtkSMProxy;
class vtkSMStateLocator;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyProperty : public vtkSMProperty
{
public:
  //@{
  /**
   * When we load ProxyManager state we want Proxy/InputProperty to be able to
   * create the corresponding missing proxy. Although when the goal is to load
   * a state on any standard proxy, we do not want that proxy property be able
   * to create new proxy based on some previous state.
   */
  static void EnableProxyCreation();
  static void DisableProxyCreation();
  static bool CanCreateProxy();
  //@}

  static vtkSMProxyProperty* New();
  vtkTypeMacro(vtkSMProxyProperty, vtkSMProperty);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Add/remove/set a proxy to the list of proxies. For SetProxy, the property
   * automatically resizes to accommodate the index specified.
   */
  virtual void AddProxy(vtkSMProxy* proxy);
  virtual void SetProxy(unsigned int idx, vtkSMProxy* proxy);
  virtual void RemoveProxy(vtkSMProxy* proxy);
  virtual void RemoveAllProxies();
  //@}

  /**
   * Sets the value of the property to the list of proxies specified.
   */
  virtual void SetProxies(unsigned int numElements, vtkSMProxy* proxies[]);

  /**
   * Returns if the given proxy is already added to the property.
   */
  bool IsProxyAdded(vtkSMProxy* proxy);

  //@{
  /**
   * Add an unchecked proxy. Does not modify the property.
   * Unchecked proxies are used by domains when verifying whether
   * a value is acceptable. To check if a value is in the domains,
   * you can do the following:
   * @verbatim
   * - RemoveAllUncheckedProxies()
   * - AddUncheckedProxy(proxy)
   * - IsInDomains()
   * @endverbatim
   */
  virtual void AddUncheckedProxy(vtkSMProxy* proxy);
  virtual void SetUncheckedProxy(unsigned int idx, vtkSMProxy* proxy);
  //@}

  /**
   * Removes all unchecked proxies.
   */
  virtual void RemoveAllUncheckedProxies();
  void ClearUncheckedElements() override { this->RemoveAllUncheckedProxies(); }

  /**
   * Returns the number of proxies.
   */
  unsigned int GetNumberOfProxies();

  /**
   * Returns the number of unchecked proxies.
   */
  unsigned int GetNumberOfUncheckedProxies();

  //@{
  /**
   * Set the number of proxies.
   */
  void SetNumberOfProxies(unsigned int count);
  void SetNumberOfUncheckedProxies(unsigned int count);
  //@}

  /**
   * Return a proxy. No bounds check is performed.
   */
  vtkSMProxy* GetProxy(unsigned int idx);

  /**
   * Return a proxy. No bounds check is performed.
   */
  vtkSMProxy* GetUncheckedProxy(unsigned int idx);

  /**
   * Copy all property values. This method behaves differently for properties
   * with vtkSMProxyListDomain and those without it. If the property has a
   * vtkSMProxyListDomain, then the property is acting as an enumeration, giving
   * user ability to pick one of the available proxies in the domain, hence a
   * `Copy` request, will find an equivalent proxy on the target's domain and
   * set that as the value of the target property.
   */
  void Copy(vtkSMProperty* src) override;

  /**
   * Update all proxies referred by this property (if any).
   */
  void UpdateAllInputs() override;

  bool IsValueDefault() override;

  /**
   * For properties that support specifying defaults in XML configuration, this
   * method will reset the property value to the default values specified in the
   * XML.
   * Simply clears the property.
   */
  void ResetToXMLDefaults() override;

protected:
  vtkSMProxyProperty();
  ~vtkSMProxyProperty() override;

  /**
   * Let the property write its content into the stream
   */
  void WriteTo(vtkSMMessage* msg) override;

  /**
   * Let the property read and set its content from the stream
   */
  void ReadFrom(const vtkSMMessage* msg, int msg_offset, vtkSMProxyLocator*) override;

  friend class vtkSMProxy;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element) override;

  /**
   * Generic method used to generate XML state
   */
  void SaveStateValues(vtkPVXMLElement* propertyElement) override;

  /**
   * Fill state property/proxy XML element with proxy info.
   * Return the created proxy XML element that has been added as a child in the
   * property definition. If prop == NULL, you must Delete yourself the result
   * otherwise prop is olding a reference to the proxy element
   */
  virtual vtkPVXMLElement* AddProxyElementState(vtkPVXMLElement* prop, unsigned int idx);
  /**
   * Updates state from an XML element. Returns 0 on failure.
   */
  int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader) override;

  /**
   * Called when a producer fires the vtkCommand::UpdateDataEvent. We update all
   * dependent domains since the data-information may have changed.
   */
  void OnUpdateDataEvent() { this->UpdateDomains(); }

  // Static flag used to know if the locator should be used to create proxy
  // or if the session should be used to find only the existing ones
  static bool CreateProxyAllowed;

  class vtkPPInternals;
  friend class vtkPPInternals;
  vtkPPInternals* PPInternals;

private:
  vtkSMProxyProperty(const vtkSMProxyProperty&) = delete;
  void operator=(const vtkSMProxyProperty&) = delete;
};

#endif
