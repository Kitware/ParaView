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

#include "vtkSMDeserializer.h"

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

//BTX
struct vtkSMStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMStateLoader : public vtkSMDeserializer
{
public:
  static vtkSMStateLoader* New();
  vtkTypeMacro(vtkSMStateLoader, vtkSMDeserializer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element.
  int LoadState(vtkPVXMLElement* rootElement);

  // Description:
  // Get/Set the proxy locator to use. Default is 
  // vtkSMProxyLocator will be used.
  void SetProxyLocator(vtkSMProxyLocator* loc);
  vtkGetObjectMacro(ProxyLocator, vtkSMProxyLocator);

protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader();

  // Description:
  // The rootElement must be the <ServerManagerState /> xml element.
  // If rootElement is not a <ServerManagerState /> element, then we try to
  // locate the first <ServerManagerState /> nested element and load that.
  // Load the state from the given root element.
  virtual int LoadStateInternal(vtkPVXMLElement* rootElement);

  // Description:
  // Called after a new proxy is created.
  // We register all created proxies.
  virtual void CreatedNewProxy(int id, vtkSMProxy* proxy);

  // Description:
  // Overridden so that when new views are to be created, we create views
  // suitable for the connection. 
  virtual vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, vtkIdType cid);

  virtual int HandleProxyCollection(vtkPVXMLElement* collectionElement);
  virtual void HandleCustomProxyDefinitions(vtkPVXMLElement* element);
  int HandleLinks(vtkPVXMLElement* linksElement);
  virtual int BuildProxyCollectionInformation(vtkPVXMLElement*);

  // Description:
  // Process the <GlobalPropertiesManagers /> element.
  int HandleGlobalPropertiesManagers(vtkPVXMLElement*);

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

  // Description:
  // Given the xml name for a view, this returns the xmlname for the view if the
  // view is to be created on the given connection.
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. 
  const char* GetViewXMLName(int connectionID, const char *xml_name);

  vtkPVXMLElement* ServerManagerStateElement;
  vtkSMProxyLocator* ProxyLocator;
private:
  vtkSMStateLoader(const vtkSMStateLoader&); // Not implemented
  void operator=(const vtkSMStateLoader&); // Not implemented

  vtkSMStateLoaderInternals* Internal;
};

#endif
