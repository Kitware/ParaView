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
/**
 * @class   vtkSMCompoundSourceProxy
 * @brief   a proxy excapsulation a pipeline of proxies.
 *
 * vtkSMCompoundSourceProxy is a proxy that allows grouping of multiple proxies.
 * vtkSMProxy has also this capability since a proxy can have sub-proxies.
 * However, vtkSMProxy does not allow public access to these proxies. The
 * only access is through exposed properties. The main reason behind this
 * is consistency. There are proxies that will not work if the program
 * accesses the sub-proxies directly. The main purpose of
 * vtkSMCompoundSourceProxy is to provide an interface to access the
 * sub-proxies. The compound proxy also maintains the connections between
 * subproxies. This makes it possible to encapsulate a pipeline into a single
 * proxy. Since vtkSMCompoundSourceProxy is a vtkSMSourceProxy, it can be
 * directly used to input to other filters, representations etc.
 * vtkSMCompoundSourceProxy provides API to exposed properties from sub proxies
 * as well as output ports of the subproxies.
*/

#ifndef vtkSMCompoundSourceProxy_h
#define vtkSMCompoundSourceProxy_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMSourceProxy.h"

class vtkSMProxyLocator;
class vtkPVXMLElement;
class vtkSMProxyManager;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMCompoundSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMCompoundSourceProxy* New();
  vtkTypeMacro(vtkSMCompoundSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a proxy to be included in this compound proxy.
   * The name must be unique to each proxy added, otherwise the previously
   * added proxy will be replaced.
   */
  void AddProxy(const char* name, vtkSMProxy* proxy);

  /**
   * Expose a property from the sub proxy (added using AddProxy).
   * Only exposed properties are accessible externally. Note that the sub proxy
   * whose property is being exposed must have been already added using
   * AddProxy().
   */
  void ExposeProperty(const char* proxyName, const char* propertyName, const char* exposedName);

  /**
   * Expose an output port from a subproxy. Exposed output ports are treated as
   * output ports of the vtkSMCompoundSourceProxy itself.
   * This method does not may the output port available. One must call
   * CreateOutputPorts().
   */
  void ExposeOutputPort(const char* proxyName, const char* portName, const char* exposedName);

  /**
   * Expose an output port from a subproxy. Exposed output ports are treated as
   * output ports of the vtkSMCompoundSourceProxy itself.
   * This method does not may the output port available. One must call
   * CreateOutputPorts().
   */
  void ExposeOutputPort(const char* proxyName, unsigned int portIndex, const char* exposedName);

  /**
   * Returns the number of sub-proxies.
   */
  unsigned int GetNumberOfProxies() { return this->GetNumberOfSubProxies(); }

  /**
   * Returns the sub proxy at a given index.
   */
  vtkSMProxy* GetProxy(unsigned int cc) { return this->GetSubProxy(cc); }

  /**
   * Returns the subproxy with the given name.
   */
  vtkSMProxy* GetProxy(const char* name) { return this->GetSubProxy(name); }

  /**
   * Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
   * not exist.
   */
  const char* GetProxyName(unsigned int index) { return this->GetSubProxyName(index); }

  /**
   * Creates the output port proxiess for this filter.
   * Each output port proxy corresponds to an actual output port on the
   * algorithm.
   */
  void CreateOutputPorts() override;

  /**
   * Creates extract selection proxies for each output port if not already
   * created. Overridden to update the selection proxies.
   */
  void CreateSelectionProxies() override;

  /**
   * Update the VTK object on the server by pushing the values of
   * all modified properties (un-modified properties are ignored).
   * If the object has not been created, it will be created first.
   */
  void UpdateVTKObjects() override;

  /**
   * This is the same as save state except it will remove all references to
   * "outside" proxies. Outside proxies are proxies that are not contained
   * in the compound proxy.  As a result, the saved state will be self
   * contained.  Returns the top element created. It is the caller's
   * responsibility to delete the returned element. If root is NULL,
   * the returned element will be a top level element.
   */
  vtkPVXMLElement* SaveDefinition(vtkPVXMLElement* root);

protected:
  vtkSMCompoundSourceProxy();
  ~vtkSMCompoundSourceProxy() override;

  /**
   * Read attributes from an XML element.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) override;

  /**
   * Call superclass' and then assigns a new executive
   * (vtkCompositeDataPipeline)
   */
  void CreateVTKObjects() override;

  // returns 1 if the value element should be written.
  // proxy property values that point to "outside" proxies
  // are not written
  int ShouldWriteValue(vtkPVXMLElement* valueElem);
  void TraverseForProperties(vtkPVXMLElement* root);
  void StripValues(vtkPVXMLElement* propertyElem);

private:
  vtkSMCompoundSourceProxy(const vtkSMCompoundSourceProxy&) = delete;
  void operator=(const vtkSMCompoundSourceProxy&) = delete;

  class vtkInternals;
  vtkInternals* CSInternals;
};

#endif
