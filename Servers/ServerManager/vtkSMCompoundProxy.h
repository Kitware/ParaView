/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompoundProxy - a proxy that can contain others
// .SECTION Description
// vtkSMCompoundProxy is a proxy that allows grouping of multiple proxies.
// vtkSMProxy has also this capability since a proxy can have sub-proxies.
// However, vtkSMProxy does not allow public access to these proxies. The
// only access is through exposed properties. The main reason behind this
// is consistency. There are proxies that will not work if the program
// accesses the sub-proxies directly. The main purpose of
// vtkSMCompoundProxy is to provide an interface to access the
// sub-proxies. It contains a main proxy and all the proxies that are added
// to the compound proxy are actually added to this proxy. This way, the
// main proxy can be any proxy type (for example a vtkSMSourceProxy). The
// compound proxy is used to access sub-proxies whereas the main proxy is
// used to access properties that are exposed by the group.


#ifndef __vtkCompoundProxy_h
#define __vtkCompoundProxy_h

#include "vtkSMProxy.h"

//BTX
struct vtkSMCompoundProxyInternals;
//ETX

class VTK_EXPORT vtkSMCompoundProxy : public vtkSMProxy
{
public:
  static vtkSMCompoundProxy* New();
  vtkTypeRevisionMacro(vtkSMCompoundProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the main proxy. All proxies are added to this proxy
  // as sub-proxies.
  void SetMainProxy(vtkSMProxy*);
  vtkGetObjectMacro(MainProxy, vtkSMProxy);

  // Description:
  // Returns an exposed or regular property from the MainProxy.
  // Convenience method.
  virtual vtkSMProperty* GetProperty(const char* name);

  // Description:
  // Calls UpdateVTKObjects() on the MainProxy.
  // Convenience method.
  virtual void UpdateVTKObjects();

  // Description:
  // Returns the property iterator from the MainProxy.
  // Convenience method.
  virtual vtkSMPropertyIterator* NewPropertyIterator();

  // Description:
  // Add a sub-proxy. If no main proxy exists, one will be
  // created (of type vtkSMProxy)
  void AddProxy(const char* name, vtkSMProxy* proxy);

  // Description:
  // Remove a sub-proxy.
  void RemoveProxy(const char* name);

  // Description:
  // Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
  vtkSMProxy* GetProxy(const char* name);

  // Description:
  // Returns a sub-proxy. Returns 0 if sub-proxy does not exist.
  vtkSMProxy* GetProxy(unsigned int index);

  // Description:
  // Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
  // not exist.
  const char* GetProxyName(unsigned int index);

  // Description:
  // Returns the number of sub-proxies.
  unsigned int GetNumberOfProxies();

  // Description:
  // Expose all main proxy properties that point to external
  // proxies.
  void ExposeExternalProperties();

  // Description:
  // Given a proxy/property pair, expose the property with the
  // given (exposed property) name.
  void ExposeProperty(const char* proxyName, 
                      const char* propertyName,
                      const char* exposedName);

  // Description:
  // This is the same as save state except it will remove all references to
  // "outside" proxies. Outside proxies are proxies that are not contained
  // in the compound proxy.  As a result, the saved state will be self
  // contained.  Returns the top element created. It is the caller's
  // responsibility to delete the returned element. If root is NULL,
  // the returned element will be a top level element.
  virtual vtkPVXMLElement* SaveDefinition(vtkPVXMLElement* root);
  
  // Description:
  // Set the connection ID.
  virtual void SetConnectionID(vtkIdType id);
protected:
  vtkSMCompoundProxy();
  ~vtkSMCompoundProxy();

  vtkSMProxy* MainProxy;

  virtual vtkSMProperty* GetProperty(const char* name, int selfOnly);

  // Description:
  // Updates state from an XML element. Returns 0 on failure.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMStateLoader* loader);
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);
  void HandleExposedProperties(vtkPVXMLElement* element);

  vtkSMCompoundProxyInternals* Internal;

  //BTX
  friend class vtkSMCompoundProxyDefinitionLoader;
  //ETX

private:
  // returns 1 if the value element should be written.
  // proxy property values that point to "outside" proxies
  // are not written
  int ShouldWriteValue(vtkPVXMLElement* valueElem);

  void TraverseForProperties(vtkPVXMLElement* root);
  void StripValues(vtkPVXMLElement* propertyElem);

  vtkSMCompoundProxy(const vtkSMCompoundProxy&); // Not implemented
  void operator=(const vtkSMCompoundProxy&); // Not implemented
};

#endif
