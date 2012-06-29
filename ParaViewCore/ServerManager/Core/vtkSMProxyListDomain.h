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
  vtkTypeMacro(vtkSMProxyListDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the number of proxies in the domain.
  unsigned int GetNumberOfProxyTypes();

  // Description:
  // Returns the xml group name for the proxy at a given index.
  const char* GetProxyGroup(unsigned int index);

  // Description:
  // Returns the xml type name for the proxy at a given index.
  const char* GetProxyName(unsigned int index);

  // Description:
  // This always returns true.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Add a proxy to the domain.
  void AddProxy(vtkSMProxy*);

  // Description:
  // Returns if the proxy is present in the domain.
  bool HasProxy(vtkSMProxy*);

  // Description:
  // Get number of proxies in the domain.
  unsigned int GetNumberOfProxies();

  // Description:
  // Get proxy at a given index.
  vtkSMProxy* GetProxy(unsigned int index);

  // Description:
  // Removes the first occurence of the \c proxy in the domain.
  // Returns if the proxy was removed.
  int RemoveProxy(vtkSMProxy* proxy);

  // Description:
  // Removes the proxy at the given index.
  // Returns if the proxy was removed.
  int RemoveProxy(unsigned int index);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  virtual int SetDefaultValues(vtkSMProperty* prop);
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

  // Description:
  // Save state for this domain.
  virtual void ChildSaveState(vtkPVXMLElement* propertyElement);

  // Load the state of the domain from the XML.
  virtual int LoadState(vtkPVXMLElement* domainElement, 
    vtkSMProxyLocator* loader); 

private:
  vtkSMProxyListDomain(const vtkSMProxyListDomain&); // Not implemented.
  void operator=(const vtkSMProxyListDomain&); // Not implemented.

  vtkSMProxyListDomainInternals* Internals;
};

#endif

