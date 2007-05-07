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

#include "vtkSMStateLoaderBase.h"

class vtkPVXMLElement;
class vtkSMProxy;

//BTX
struct vtkSMStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMStateLoader : public vtkSMStateLoaderBase
{
public:
  static vtkSMStateLoader* New();
  vtkTypeRevisionMacro(vtkSMStateLoader, vtkSMStateLoaderBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element. This root
  // element must have Proxy and ProxyCollection sub-elements
  // Returns 1 on success, 0 on failure.
  // If keep_proxies is set, then the internal map
  // of proxy ids to proxies is not cleared on loading of the state.
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader();

  vtkPVXMLElement* RootElement;
  void SetRootElement(vtkPVXMLElement*);

  virtual int HandleProxyCollection(vtkPVXMLElement* collectionElement);
  virtual void HandleCompoundProxyDefinitions(vtkPVXMLElement* element);
  int HandleLinks(vtkPVXMLElement* linksElement);
  virtual int BuildProxyCollectionInformation(vtkPVXMLElement*);

  // Description:
  // Called after a new proxy is created.
  // We register all created proxies.
  virtual void CreatedNewProxy(int id, vtkSMProxy* proxy);

  // Description:
  // This method scans through the internal data structure built 
  // during BuildProxyCollectionInformation() and registers the proxy. 
  // The DS keeps info
  // about each proxy ID and the groups and names 
  // the proxy should be registered as (as indicated in the state file).
  virtual void RegisterProxy(int id, vtkSMProxy* proxy);
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  // Description:
  // Return the xml element for the state of the proxy with the given id.
  // This is used by NewProxy() when the proxy with the given id
  // is not located in the internal CreatedProxies map.
  virtual vtkPVXMLElement* LocateProxyElement(int id);

  // Description:
  // Used by LocateProxyElement(). Recursively tries to locate the
  // proxy state element for the proxy.
  vtkPVXMLElement* LocateProxyElementInternal(vtkPVXMLElement* root, int id);

  // Description:
  // Checks the root element for version. If failed, return false.
  virtual bool VerifyXMLVersion(vtkPVXMLElement* rootElement);

private:
  vtkSMStateLoader(const vtkSMStateLoader&); // Not implemented
  void operator=(const vtkSMStateLoader&); // Not implemented

  vtkSMStateLoaderInternals* Internal;
};

#endif
