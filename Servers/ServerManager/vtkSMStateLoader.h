/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLoader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateLoader - Utility class to load state from XML
// .SECTION Description
// vtkSMStateLoader can load server manager state from a given 
// vtkPVXMLElement. This element is usually populated by a vtkPVXMLParser.
// .SECTION See Also
// vtkPVXMLParser vtkPVXMLElement

#ifndef __vtkSMStateLoader_h
#define __vtkSMStateLoader_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

//BTX
struct vtkSMStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMStateLoader : public vtkSMObject
{
public:
  static vtkSMStateLoader* New();
  vtkTypeRevisionMacro(vtkSMStateLoader, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element. This root
  // element must have Proxy and ProxyCollection sub-elements
  // Returns 1 on success, 0 on failure.
  // If keep_proxies is set, then the internal map
  // of proxy ids to proxies is not cleared on loading of the state.
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Description:
  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key.
  vtkSMProxy* NewProxy(int id);
  vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);

protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader();

  vtkPVXMLElement* RootElement;

  void ClearCreatedProxies();

  int HandleProxyCollection(vtkPVXMLElement* collectionElement);
  void HandleCompoundProxyDefinitions(vtkPVXMLElement* element);
  int HandleLinks(vtkPVXMLElement* linksElement);

  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key. The root is the xml
  // element under which the proxy definitions are stored.
  vtkSMProxy* NewProxy(vtkPVXMLElement* root, int id);

  vtkSMStateLoaderInternals* Internal;

private:
  vtkSMStateLoader(const vtkSMStateLoader&); // Not implemented
  void operator=(const vtkSMStateLoader&); // Not implemented
};

#endif
