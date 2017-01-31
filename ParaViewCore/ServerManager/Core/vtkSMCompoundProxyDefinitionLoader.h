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

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDeserializerXML.h"

class vtkPVXMLElement;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCompoundProxyDefinitionLoader : public vtkSMDeserializerXML
{
public:
  static vtkSMCompoundProxyDefinitionLoader* New();
  vtkTypeMacro(vtkSMCompoundProxyDefinitionLoader, vtkSMDeserializerXML);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual void SetRootElement(vtkPVXMLElement*);

protected:
  vtkSMCompoundProxyDefinitionLoader();
  ~vtkSMCompoundProxyDefinitionLoader();

  /**
   * Locate the XML for the proxy with the given id.
   */
  virtual vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id) VTK_OVERRIDE;

  vtkPVXMLElement* RootElement;

private:
  vtkSMCompoundProxyDefinitionLoader(const vtkSMCompoundProxyDefinitionLoader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMCompoundProxyDefinitionLoader&) VTK_DELETE_FUNCTION;
};

#endif
