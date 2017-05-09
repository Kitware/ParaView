/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyListDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyListDomain
 * @brief   union of proxies.
 *
 * This domain is a collection of proxies that can be assigned as the value
 * to a vtkSMProxyProperty.
 * The Server Mananger configuration defines the proxy types that form this list,
 * while the value of this domain is the list of instances of proxies.
 * Example usage :
 *
 * \code{.xml}
 * <ProxyListDomain name="proxy_list">
 *   <Proxy group="implicit_functions"
 *          name="Plane" />
 *   <Group name="implicit_functions"/>
 * </ProxyListDomain>
 * \endcode
 *
 * @sa
 * vtkSMDomain vtkSMProxyProperty
*/

#ifndef vtkSMProxyListDomain_h
#define vtkSMProxyListDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"

class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyListDomainInternals;
class vtkSMSessionProxyManager;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyListDomain : public vtkSMDomain
{
public:
  static vtkSMProxyListDomain* New();
  vtkTypeMacro(vtkSMProxyListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns the number of proxies in the domain.
   */
  unsigned int GetNumberOfProxyTypes();

  /**
   * Returns the xml group name for the proxy at a given index.
   */
  const char* GetProxyGroup(unsigned int index);

  /**
   * Returns the xml type name for the proxy at a given index.
   */
  const char* GetProxyName(unsigned int index);

  /**
   * If the \c proxy is part of the domain, then this returns the name used for
   * the proxy in the domain. Returns NULL otherwise.
   */
  const char* GetProxyName(vtkSMProxy* proxy);

  /**
   * Inverse of `GetProxyName`, returns the first proxy with the given name.
   */
  vtkSMProxy* GetProxyWithName(const char* pname);

  /**
   * This always returns true.
   */
  virtual int IsInDomain(vtkSMProperty* property) VTK_OVERRIDE;

  /**
   * Add a proxy to the domain.
   */
  void AddProxy(vtkSMProxy*);

  /**
   * Returns if the proxy is present in the domain.
   */
  bool HasProxy(vtkSMProxy*);

  /**
   * Get number of proxies in the domain.
   */
  unsigned int GetNumberOfProxies();

  /**
   * Get proxy at a given index.
   */
  vtkSMProxy* GetProxy(unsigned int index);

  /**
   * Find a proxy in the domain of the given group and type.
   */
  vtkSMProxy* FindProxy(const char* xmlgroup, const char* xmlname);

  /**
   * Removes the first occurence of the \c proxy in the domain.
   * Returns if the proxy was removed.
   */
  int RemoveProxy(vtkSMProxy* proxy);

  /**
   * Removes the proxy at the given index.
   * Returns if the proxy was removed.
   */
  int RemoveProxy(unsigned int index);

  /**
   * Creates and populates the domain with the proxy-types. This will remove any
   * existing proxies in the domain. Note that the newly created proxies won't
   * be registered with the proxy manager.
   */
  void CreateProxies(vtkSMSessionProxyManager* pxm);

  //@{
  /**
   * A vtkSMProperty is often defined with a default value in the
   * XML itself. However, many times, the default value must be determined
   * at run time. To facilitate this, domains can override this method
   * to compute and set the default value for the property.
   * Note that unlike the compile-time default values, the
   * application must explicitly call this method to initialize the
   * property.
   */
  virtual int SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values) VTK_OVERRIDE;

protected:
  vtkSMProxyListDomain();
  ~vtkSMProxyListDomain();
  //@}

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  /**
   * Adds a proxy type, used by ReadXMLAttributes().
   */
  void AddProxy(const char* group, const char* name);

  /**
   * Save state for this domain.
   */
  virtual void ChildSaveState(vtkPVXMLElement* propertyElement) VTK_OVERRIDE;

  // Load the state of the domain from the XML.
  virtual int LoadState(vtkPVXMLElement* domainElement, vtkSMProxyLocator* loader) VTK_OVERRIDE;

  friend class vtkSMProxyProperty;
  void SetProxies(vtkSMProxy** proxies, unsigned int count);

private:
  vtkSMProxyListDomain(const vtkSMProxyListDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMProxyListDomain&) VTK_DELETE_FUNCTION;

  vtkSMProxyListDomainInternals* Internals;
};

#endif
