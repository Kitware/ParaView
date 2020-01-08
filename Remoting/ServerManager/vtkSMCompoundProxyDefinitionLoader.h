/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundProxyDefinitionLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCompoundProxyDefinitionLoader
 * @brief   Creates a compound proxy from an
 * XML definition.
 *
 * vtkSMCompoundProxyDefinitionLoader can load a compound proxy definition
 * from a given vtkPVXMLElement. This element can be populated by a
 * vtkPVXMLElement or obtained from the proxy manager.
 * @sa
 * vtkPVXMLElement vtkPVXMLParser vtkSMProxyManager
*/

#ifndef vtkSMCompoundProxyDefinitionLoader_h
#define vtkSMCompoundProxyDefinitionLoader_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDeserializerXML.h"

class vtkPVXMLElement;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMCompoundProxyDefinitionLoader
  : public vtkSMDeserializerXML
{
public:
  static vtkSMCompoundProxyDefinitionLoader* New();
  vtkTypeMacro(vtkSMCompoundProxyDefinitionLoader, vtkSMDeserializerXML);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void SetRootElement(vtkPVXMLElement*);

protected:
  vtkSMCompoundProxyDefinitionLoader();
  ~vtkSMCompoundProxyDefinitionLoader() override;

  /**
   * Locate the XML for the proxy with the given id.
   */
  vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id) override;

  vtkPVXMLElement* RootElement;

private:
  vtkSMCompoundProxyDefinitionLoader(const vtkSMCompoundProxyDefinitionLoader&) = delete;
  void operator=(const vtkSMCompoundProxyDefinitionLoader&) = delete;
};

#endif
