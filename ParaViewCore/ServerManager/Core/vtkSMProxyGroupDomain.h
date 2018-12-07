/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyGroupDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMProxyGroupDomain
 * @brief   union of proxy groups
 *
 * The proxy group domain consists of all proxies in a list of groups.
 * This domain is commonly used together with vtkSMProxyproperty
 * Valid XML elements are:
 * @verbatim
 * * <Group name=""> where name is the groupname used by the proxy
 * manager to refer to a group of proxies.
 * @endverbatim// .SECTION See Also
 * vtkSMDomain vtkSMProxyproperty
*/

#ifndef vtkSMProxyGroupDomain_h
#define vtkSMProxyGroupDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDomain.h"

class vtkSMProperty;
class vtkSMProxy;

struct vtkSMProxyGroupDomainInternals;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMProxyGroupDomain : public vtkSMDomain
{
public:
  static vtkSMProxyGroupDomain* New();
  vtkTypeMacro(vtkSMProxyGroupDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a group to the domain. The domain is the union of
   * all groups.
   */
  void AddGroup(const char* group);

  /**
   * Returns true if the value of the property is in the domain.
   * The property has to be a vtkSMProxyproperty or a sub-class. All
   * proxies pointed by the property have to be in the domain.
   */
  int IsInDomain(vtkSMProperty* property) override;

  /**
   * Returns true if the proxy is in the domain.
   */
  int IsInDomain(vtkSMProxy* proxy);

  /**
   * Returns the number of groups.
   */
  unsigned int GetNumberOfGroups();

  /**
   * Returns group with give id. Does not perform bounds check.
   */
  const char* GetGroup(unsigned int idx);

  /**
   * Returns the total number of proxies in the domain.
   */
  unsigned int GetNumberOfProxies();

  /**
   * Given a name, returns a proxy.
   */
  vtkSMProxy* GetProxy(const char* name);

  /**
   * Returns the name (in the group) of a proxy.
   */
  const char* GetProxyName(unsigned int idx);

  /**
   * Returns the name (in the group) of a proxy.
   */
  const char* GetProxyName(vtkSMProxy* proxy);

protected:
  vtkSMProxyGroupDomain();
  ~vtkSMProxyGroupDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  vtkSMProxyGroupDomainInternals* PGInternals;

private:
  vtkSMProxyGroupDomain(const vtkSMProxyGroupDomain&) = delete;
  void operator=(const vtkSMProxyGroupDomain&) = delete;
};

#endif
