/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCompoundSourceProxy - a proxy excapsulation a pipeline of proxies.
// .SECTION Description
// vtkSMCompoundSourceProxy is a proxy that allows grouping of multiple proxies.
// vtkSMProxy has also this capability since a proxy can have sub-proxies.
// However, vtkSMProxy does not allow public access to these proxies. The
// only access is through exposed properties. The main reason behind this
// is consistency. There are proxies that will not work if the program
// accesses the sub-proxies directly. The main purpose of
// vtkSMCompoundSourceProxy is to provide an interface to access the
// sub-proxies. The compound proxy also maintains the connections between
// subproxies. This makes it possible to encapsulate a pipeline into a single
// proxy. Since vtkSMCompoundSourceProxy is a vtkSMSourceProxy, it can be
// directly used to input to other filters, representations etc.
// vtkSMCompoundSourceProxy provides API to exposed properties from sub proxies
// as well as output ports of the subproxies.


#ifndef __vtkSMCompoundSourceProxy_h
#define __vtkSMCompoundSourceProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMCompoundSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMCompoundSourceProxy* New();
  vtkTypeMacro(vtkSMCompoundSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a proxy to be included in this compound proxy.
  // The name must be unique to each proxy added, otherwise the previously
  // added proxy will be replaced.
  void AddProxy(const char* name, vtkSMProxy* proxy);

  // Description:
  // Expose a property from the sub proxy (added using AddProxy).
  // Only exposed properties are accessible externally. Note that the sub proxy
  // whose property is being exposed must have been already added using
  // AddProxy().
  void ExposeProperty(const char* proxyName, 
                      const char* propertyName,
                      const char* exposedName);

  // Description:
  // Expose an output port from a subproxy. Exposed output ports are treated as
  // output ports of the vtkSMCompoundSourceProxy itself. 
  // This method does not may the output port available. One must call
  // CreateOutputPorts().
  void ExposeOutputPort(const char* proxyName, 
                        const char* portName,
                        const char* exposedName);

  // Description:
  // Expose an output port from a subproxy. Exposed output ports are treated as
  // output ports of the vtkSMCompoundSourceProxy itself. 
  // This method does not may the output port available. One must call
  // CreateOutputPorts().
  void ExposeOutputPort(const char* proxyName,
                        unsigned int portIndex,
                        const char* exposedName);

  // Description:
  // Returns the number of sub-proxies.
  unsigned int GetNumberOfProxies()
    { return this->GetNumberOfSubProxies(); }

  // Description:
  // Returns the sub proxy at a given index.
  vtkSMProxy* GetProxy(unsigned int cc)
    { return this->GetSubProxy(cc); }

  // Description:
  // Returns the subproxy with the given name.
  vtkSMProxy* GetProxy(const char* name)
    { return this->GetSubProxy(name); }

  // Description:
  // Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
  // not exist.
  const char* GetProxyName(unsigned int index)
    { return this->GetSubProxyName(index); }

  // Description:
  // This is the same as save state except it will remove all references to
  // "outside" proxies. Outside proxies are proxies that are not contained
  // in the compound proxy.  As a result, the saved state will be self
  // contained.  Returns the top element created. It is the caller's
  // responsibility to delete the returned element. If root is NULL,
  // the returned element will be a top level element.
  vtkPVXMLElement* SaveDefinition(vtkPVXMLElement* root);

  // Description:
  // Creates the output port proxiess for this filter. 
  // Each output port proxy corresponds to an actual output port on the
  // algorithm.
  virtual void CreateOutputPorts();

  // Description:
  // Overloaded to ensure that the chain of subproxies is updated in correct
  // sequence. 
  virtual void UpdateVTKObjects()
    { this->Superclass::UpdateVTKObjects(); }

  // Description:
  // This method saves state information about the proxy
  // which can be used to revive the proxy using server side objects
  // already present. This includes the entire state saved by calling 
  // SaveState() as well additional information such as server side
  // object IDs.
  // Overridden to avoid saving information about output ports, since the output
  // ports of a compound proxy don't really belong to the compound proxy itself.
  virtual vtkPVXMLElement* SaveRevivalState(vtkPVXMLElement* root);
  virtual int LoadRevivalState(vtkPVXMLElement* revivalElement);

//BTX
protected:
  vtkSMCompoundSourceProxy();
  ~vtkSMCompoundSourceProxy();

  // Description:
  // Overloaded to ensure that the chain of subproxies is updated in correct
  // sequence. 
  virtual void UpdateVTKObjects(vtkClientServerStream&);

  // Description:
  // Given a class name (by setting VTKClassName) and server ids (by
  // setting ServerIDs), this methods instantiates the objects on the
  // server(s)
  virtual void CreateVTKObjects();
  
  friend class vtkSMCompoundProxyDefinitionLoader;

  // Description:
  // Load the compound proxy definition.
  int LoadDefinition(vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator);

private:
  vtkSMCompoundSourceProxy(const vtkSMCompoundSourceProxy&); // Not implemented
  void operator=(const vtkSMCompoundSourceProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* CSInternal;

  // returns 1 if the value element should be written.
  // proxy property values that point to "outside" proxies
  // are not written
  int ShouldWriteValue(vtkPVXMLElement* valueElem);
  void TraverseForProperties(vtkPVXMLElement* root);
  void StripValues(vtkPVXMLElement* propertyElem);
  void HandleExposedProperties(vtkPVXMLElement* element);
//ETX
};

#endif

