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
// .NAME vtkSMCompoundProxyDefinitionLoader - Creates a compound proxy from an XML definition
// .SECTION Description
// vtkSMCompoundProxyDefinitionLoader can load a compound proxy definition
// from a given vtkPVXMLElement. This element can be populated by a 
// vtkPVXMLElement or obtained from the proxy manager.
// .SECTION See Also
// vtkPVXMLElement vtkPVXMLParser vtkSMProxyManager

#ifndef __vtkSMCompoundProxyDefinitionLoader_h
#define __vtkSMCompoundProxyDefinitionLoader_h

#include "vtkSMStateLoaderBase.h"

class vtkSMCompoundProxy;

class VTK_EXPORT vtkSMCompoundProxyDefinitionLoader : public vtkSMStateLoaderBase
{
public:
  static vtkSMCompoundProxyDefinitionLoader* New();
  vtkTypeRevisionMacro(vtkSMCompoundProxyDefinitionLoader, 
    vtkSMStateLoaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load a compound proxy definition from an xml element.
  // Returns the created compound proxy or NULL (on failure)
  vtkSMCompoundProxy* LoadDefinition(vtkPVXMLElement* rootElement);

protected:
  vtkSMCompoundProxyDefinitionLoader();
  ~vtkSMCompoundProxyDefinitionLoader();

  // Description:
  // Called after a new proxy is created.
  virtual void CreatedNewProxy(int vtkNotUsed(id), vtkSMProxy* vtkNotUsed(proxy))
    { }

  vtkSMCompoundProxy* HandleDefinition(vtkPVXMLElement* rootElement);

private:
  vtkSMCompoundProxyDefinitionLoader(const vtkSMCompoundProxyDefinitionLoader&); // Not implemented
  void operator=(const vtkSMCompoundProxyDefinitionLoader&); // Not implemented
};

#endif
