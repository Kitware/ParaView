/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCompoundSourceProxyDefinitionBuilder
 * @brief   used to build a
 * vtkSMCompoundSourceProxy definition.
 *
 * vtkSMCompoundSourceProxyDefinitionBuilder is used to create a XML definition
 * for a compound-proxy consisting of other proxies. This class can only build
 * one compound-proxy definition at a time. Use Reset() to start a new
 * definition.
 * @sa
 * vtkSMCompoundSourceProxy
*/

#ifndef vtkSMCompoundSourceProxyDefinitionBuilder_h
#define vtkSMCompoundSourceProxyDefinitionBuilder_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMProxy;
class vtkPVXMLElement;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCompoundSourceProxyDefinitionBuilder : public vtkSMObject
{
public:
  static vtkSMCompoundSourceProxyDefinitionBuilder* New();
  vtkTypeMacro(vtkSMCompoundSourceProxyDefinitionBuilder, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Resets the builder. This can be used when using the builder to create
   * multiple definitions.
   */
  void Reset();

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
  unsigned int GetNumberOfProxies();

  /**
   * Returns the sub proxy at a given index.
   */
  vtkSMProxy* GetProxy(unsigned int cc);

  /**
   * Returns the subproxy with the given name.
   */
  vtkSMProxy* GetProxy(const char* name);

  /**
   * Returns the name used to store sub-proxy. Returns 0 if sub-proxy does
   * not exist.
   */
  const char* GetProxyName(unsigned int index);

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
  vtkSMCompoundSourceProxyDefinitionBuilder();
  ~vtkSMCompoundSourceProxyDefinitionBuilder() override;

private:
  vtkSMCompoundSourceProxyDefinitionBuilder(
    const vtkSMCompoundSourceProxyDefinitionBuilder&) = delete;
  void operator=(const vtkSMCompoundSourceProxyDefinitionBuilder&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  // returns 1 if the value element should be written.
  // proxy property values that point to "outside" proxies
  // are not written
  int ShouldWriteValue(vtkPVXMLElement* valueElem);
  void TraverseForProperties(vtkPVXMLElement* root);
  void StripValues(vtkPVXMLElement* propertyElem);
};

#endif
