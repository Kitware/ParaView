/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelfGeneratingSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSelfGeneratingSourceProxy
 * @brief source proxy that generates its own proxy definition at run time.
 *
 *
 * vtkSMSelfGeneratingSourceProxy is a source proxy that supports extending its property
 * definition at runtime. Client code can instantiate this proxy using standard
 * mechanisms and then call `ExtendDefinition` to add XML stubs to this proxy's
 * definitions. vtkSMSelfGeneratingSourceProxy (working together with
 * vtkSIProxy) ensures that those extensions get loaded correctly on both client
 * and server side. After that point, the proxy is pretty much like a regular
 * proxy together with its properties.
 *
 * vtkSMSelfGeneratingSourceProxy also ensures that when the XML state for this
 * proxy gets saved, the extended definitions are also saved in the XML state so
 * that they can be loaded back as well.
 *
 * @warning
 * This is only intended for simple source proxies. The `ExtendDefinition()` API
 * is only intended to add new property definitions for the proxy and should not
 * be used for adding other entities in a proxy definition such as sub proxies,
 * hints, documentation, etc.
 */

#ifndef vtkSMSelfGeneratingSourceProxy_h
#define vtkSMSelfGeneratingSourceProxy_h

#include "vtkSMSourceProxy.h"

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMSelfGeneratingSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMSelfGeneratingSourceProxy* New();
  vtkTypeMacro(vtkSMSelfGeneratingSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Will extend this proxy to add properties using the XML definition provided.
   */
  virtual bool ExtendDefinition(const char* proxy_definition_xml);
  virtual bool ExtendDefinition(vtkPVXMLElement* xml);
  //@}

  //@{
  /**
   * Overridden to save information about extended definitions loaded.
   */
  vtkPVXMLElement* SaveXMLState(vtkPVXMLElement* root, vtkSMPropertyIterator* iter) override;
  using Superclass::SaveXMLState;
  //@}

  /**
   * Overridden to process extended definition XML in the state file.
   */
  int LoadXMLState(vtkPVXMLElement* element, vtkSMProxyLocator* locator) override;

  /**
   * Overridden to push extended definitions to the server side if not already pushed.
   */
  void CreateVTKObjects() override;

protected:
  vtkSMSelfGeneratingSourceProxy();
  ~vtkSMSelfGeneratingSourceProxy() override;

private:
  vtkSMSelfGeneratingSourceProxy(const vtkSMSelfGeneratingSourceProxy&) = delete;
  void operator=(const vtkSMSelfGeneratingSourceProxy&) = delete;

  bool ExtendDefinitionOnSIProxy(vtkPVXMLElement* xml);

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
