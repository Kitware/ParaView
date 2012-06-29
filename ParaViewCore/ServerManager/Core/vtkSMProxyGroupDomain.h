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
// .NAME vtkSMProxyGroupDomain - union of proxy groups
// .SECTION Description
// The proxy group domain consists of all proxies in a list of groups.
// This domain is commonly used together with vtkSMProxyPropery
// Valid XML elements are:
// @verbatim
// * <Group name=""> where name is the groupname used by the proxy
// manager to refer to a group of proxies.
// @endverbatim// .SECTION See Also
// vtkSMDomain vtkSMProxyPropery

#ifndef __vtkSMProxyGroupDomain_h
#define __vtkSMProxyGroupDomain_h

#include "vtkSMDomain.h"

class vtkSMProperty;
class vtkSMProxy;
//BTX
struct vtkSMProxyGroupDomainInternals;
//ETX

class VTK_EXPORT vtkSMProxyGroupDomain : public vtkSMDomain
{
public:
  static vtkSMProxyGroupDomain* New();
  vtkTypeMacro(vtkSMProxyGroupDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a group to the domain. The domain is the union of
  // all groups.
  void AddGroup(const char* group);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyPropery or a sub-class. All 
  // proxies pointed by the property have to be in the domain.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the proxy is in the domain.
  int IsInDomain(vtkSMProxy* proxy);

  // Description:
  // Returns the number of groups.
  unsigned int GetNumberOfGroups();

  // Description:
  // Returns group with give id. Does not perform bounds check.
  const char* GetGroup(unsigned int idx);

  // Description:
  // Returns the total number of proxies in the domain.
  unsigned int GetNumberOfProxies();

  // Description:
  // Given a name, returns a proxy.
  vtkSMProxy* GetProxy(const char* name);

  // Description:
  // Returns the name (in the group) of a proxy.
  const char* GetProxyName(unsigned int idx);

  // Description:
  // Returns the name (in the group) of a proxy.
  const char* GetProxyName(vtkSMProxy* proxy);

protected:
  vtkSMProxyGroupDomain();
  ~vtkSMProxyGroupDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  vtkSMProxyGroupDomainInternals* PGInternals;

private:
  vtkSMProxyGroupDomain(const vtkSMProxyGroupDomain&); // Not implemented
  void operator=(const vtkSMProxyGroupDomain&); // Not implemented
};

#endif
