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
 * This domain is commonly used together with vtkSMProxyPropery
 * Valid XML elements are:
 * @verbatim
 * * <Group name=""> where name is the groupname used by the proxy
 * manager to refer to a group of proxies.
 * @endverbatim// .SECTION See Also
 * vtkSMDomain vtkSMProxyPropery
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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a group to the domain. The domain is the union of
   * all groups.
   */
  void AddGroup(const char* group);

  /**
   * Returns true if the value of the propery is in the domain.
   * The propery has to be a vtkSMProxyPropery or a sub-class. All
   * proxies pointed by the property have to be in the domain.
   */
  virtual int IsInDomain(vtkSMProperty* property) VTK_OVERRIDE;

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
  ~vtkSMProxyGroupDomain();

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  vtkSMProxyGroupDomainInternals* PGInternals;

private:
  vtkSMProxyGroupDomain(const vtkSMProxyGroupDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMProxyGroupDomain&) VTK_DELETE_FUNCTION;
};

#endif
