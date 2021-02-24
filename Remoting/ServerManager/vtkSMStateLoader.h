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
/**
 * @class   vtkSMStateLoader
 * @brief   Utility class to load state from XML
 *
 * vtkSMStateLoader can load server manager state from a given
 * vtkPVXMLElement. This element is usually populated by a vtkPVXMLParser.
 * @sa
 * vtkPVXMLParser vtkPVXMLElement
*/

#ifndef vtkSMStateLoader_h
#define vtkSMStateLoader_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMDeserializerXML.h"

#include <map>    // needed for API
#include <string> // needed for API

class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

struct vtkSMStateLoaderInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMStateLoader : public vtkSMDeserializerXML
{
public:
  static vtkSMStateLoader* New();
  vtkTypeMacro(vtkSMStateLoader, vtkSMDeserializerXML);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Load the state from the given root element.
   */
  int LoadState(vtkPVXMLElement* rootElement, bool keepOriginalId = false);

  //@{
  /**
   * Get/Set the proxy locator to use. Default is
   * vtkSMProxyLocator will be used.
   */
  void SetProxyLocator(vtkSMProxyLocator* loc);
  vtkGetObjectMacro(ProxyLocator, vtkSMProxyLocator);
  //@}

  //@{
  /**
   * Allow the loader to keep the mapping between the id from the loaded state
   * to the current proxy attributed id.
   */
  vtkSetMacro(KeepIdMapping, int);
  vtkGetMacro(KeepIdMapping, int);
  vtkBooleanMacro(KeepIdMapping, int);
  //@}

  //@{
  /**
   * Return an array of ids. The ids are stored in the following order
   * and the size of the array is provided as argument.
   * [key, value, key, value, ...]
   * The array is kept internally using a std::vector
   */
  vtkTypeUInt32* GetMappingArray(int& size);

protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader() override;
  //@}

  /**
   * The rootElement must be the \c \<ServerManagerState/\> xml element.
   * If rootElement is not a \c \<ServerManagerState/\> element, then we try to
   * locate the first \c \<ServerManagerState/\> nested element and load that.
   * Load the state from the given root element.
   */
  virtual int LoadStateInternal(vtkPVXMLElement* rootElement);

  /**
   * Called after a new proxy is created. The main responsibility of this method
   * is to ensure that proxy gets a GlobalId (either automatically or using the
   * id from the state if LoadState() was called with \c keepOriginalId set to
   * true). It also called vtkSMProxy::UpdateVTKObjects() and
   * vtkSMProxy::UpdatePipelineInformation() (if applicable) to ensure that the
   * state loaded on the proxy is "pushed" and any info properties updated.
   * We also create a list to track the order in which proxies are created.
   * This order is a dependency order too and hence helps us register proxies in
   * order of dependencies.
   */
  void CreatedNewProxy(vtkTypeUInt32 id, vtkSMProxy* proxy) override;

  /**
   * Overridden so that when new views are to be created, we create views
   * suitable for the connection.
   */
  vtkSMProxy* CreateProxy(
    const char* xmlgroup, const char* xmlname, const char* subProxyName = nullptr) override;

  virtual int HandleProxyCollection(vtkPVXMLElement* collectionElement);
  virtual void HandleCustomProxyDefinitions(vtkPVXMLElement* element);
  int HandleLinks(vtkPVXMLElement* linksElement);
  virtual int BuildProxyCollectionInformation(vtkPVXMLElement*);

  //@{
  /**
   * This method scans through the internal data structure built
   * during BuildProxyCollectionInformation() and registers the proxy.
   * The DS keeps info
   * about each proxy ID and the groups and names
   * the proxy should be registered as (as indicated in the state file).
   */
  virtual void RegisterProxy(vtkTypeUInt32 id, vtkSMProxy* proxy);
  virtual void RegisterProxyInternal(const char* group, const char* name, vtkSMProxy* proxy);
  //@}

  /**
   * Update a proxy's registration group and name before it gets registered.
   * Default implementation handles helper group group names.
   * Returns false to skip registering the proxy.
   * @param[in,out] group  proxy registration group
   * @param[in,out] name   proxy registration name
   * @param[in]     proxy  proxy being registered
   * @returns true to continue registering the proxy, false to skip registering it.
   */
  virtual bool UpdateRegistrationInfo(std::string& group, std::string& name, vtkSMProxy* proxy);

  /**
   * Return the xml element for the state of the proxy with the given id.
   * This is used by NewProxy() when the proxy with the given id
   * is not located in the internal CreatedProxies map.
   */
  vtkPVXMLElement* LocateProxyElement(vtkTypeUInt32 id) override;

  /**
   * Used by LocateProxyElement(). Recursively tries to locate the
   * proxy state element for the proxy.
   */
  vtkPVXMLElement* LocateProxyElementInternal(vtkPVXMLElement* root, vtkTypeUInt32 id);

  /**
   * Checks the root element for version. If failed, return false.
   */
  virtual bool VerifyXMLVersion(vtkPVXMLElement* rootElement);

  /**
   * Returns the first proxy already registered using any of the registration
   * (group, name) assigned to the proxy with the given id in the state file.
   */
  vtkSMProxy* LocateExistingProxyUsingRegistrationName(vtkTypeUInt32 id);

  vtkPVXMLElement* ServerManagerStateElement;
  vtkSMProxyLocator* ProxyLocator;
  int KeepIdMapping;

private:
  vtkSMStateLoader(const vtkSMStateLoader&) = delete;
  void operator=(const vtkSMStateLoader&) = delete;

  vtkSMStateLoaderInternals* Internal;
};

#endif
