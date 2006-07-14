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
// .NAME vtkSMProxyListDomain - union of proxies.
// .SECTION Description
// This domain is a collection of proxies that can be assigned as the value
// to a vtkSMProxyProperty. 
// The Server Mananger configuration defines the proxy types that form this list,
// while the value of this domain is the list of instances of proxies.
//
// .SECTION See Also
// vtkSMDomain vtkSMProxyProperty

#ifndef __vtkSMProxyListDomain_h
#define __vtkSMProxyListDomain_h

#include "vtkSMDomain.h"

class vtkSMProperty;
class vtkSMProxy;
class vtkSMProxyListDomainInternals;

class VTK_EXPORT vtkSMProxyListDomain : public vtkSMDomain
{
public:
  static vtkSMProxyListDomain* New();
  vtkTypeRevisionMacro(vtkSMProxyListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Populates the domain by creating new instances of the proxy
  // types specified in the configuration. This clears any 
  // already existing proxies in the domain.
  void CreateProxyList(vtkIdType connectionId);

  // Description:
  // Returns the number of proxies in the domain.
  unsigned int GetNumberOfProxies();

  // Description:
  // Returns the proxy at a given index.
  vtkSMProxy* GetProxy(unsigned int index);

  int GetIndex(vtkSMProxy* proxy);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMProxyPropery or a sub-class. All 
  // proxies pointed by the property have to be in the domain.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the proxy is in the domain.
  int IsInDomain(vtkSMProxy* proxy);
protected:
  vtkSMProxyListDomain();
  ~vtkSMProxyListDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Description:
  // Adds a proxy type, used by ReadXMLAttributes().
  void AddProxy(const char* group, const char* name);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);


  virtual int LoadState(vtkPVXMLElement* domainElement, 
    vtkSMStateLoader* loader);
private:
  vtkSMProxyListDomain(const vtkSMProxyListDomain&); // Not implemented.
  void operator=(const vtkSMProxyListDomain&); // Not implemented.

  vtkSMProxyListDomainInternals* Internals;
};

#endif

