/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyManagerUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMProxyManagerUtilities
 * @brief miscellaneous collection of proxy-manager utilities
 *
 * vtkSMProxyManagerUtilities provides collection of APIs useful with dealing
 * with proxies registered with the proxy manager.
 *
 * @sa vtkSMSessionProxyManager
 */

#ifndef vtkSMProxyManagerUtilities_h
#define vtkSMProxyManagerUtilities_h

#include "vtkRemotingServerManagerModule.h" // needed for exports
#include "vtkSMObject.h"

#include <map>    // for std::map
#include <set>    // for std::set
#include <string> // for std::string

class vtkPVXMLElement;
class vtkSMSessionProxyManager;
class vtkSMProxy;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxyManagerUtilities : public vtkSMObject
{
public:
  static vtkSMProxyManagerUtilities* New();
  vtkTypeMacro(vtkSMProxyManagerUtilities, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the proxy manager to use.
   */
  void SetProxyManager(vtkSMSessionProxyManager* pxm);
  vtkGetObjectMacro(ProxyManager, vtkSMSessionProxyManager);
  //@}

  //@{
  /**
   * Returns a collection of proxies that have the specified annotations.
   *
   * If multiple annotations are specified, the `match_all` flag it used to
   * determine if all the annotations are to be matched or match with any of the
   * annotations is sufficient.
   * Alternatively, use `GetProxiesWithAllAnnotations` or
   * `GetProxiesWithAnyAnnotations`.
   */
  std::set<vtkSMProxy*> GetProxiesWithAnnotations(
    const std::map<std::string, std::string>& annotations, bool match_all);
  std::set<vtkSMProxy*> GetProxiesWithAllAnnotations(
    const std::map<std::string, std::string>& annotations)
  {
    return this->GetProxiesWithAnnotations(annotations, true);
  }
  std::set<vtkSMProxy*> GetProxiesWithAnyAnnotations(
    const std::map<std::string, std::string>& annotations)
  {
    return this->GetProxiesWithAnnotations(annotations, false);
  }
  //@}

  //@{
  /**
   * Given a set of proxies, scans for all "dependent" proxies and returns a set
   * that includes the proxies together with all the dependent proxies. These proxies
   * are proxies that are used as helper proxies or proxies set on proxy properties.
   */
  std::set<vtkSMProxy*> CollectHelpersAndRelatedProxies(const std::set<vtkSMProxy*>& proxies);
  //@}

protected:
  vtkSMProxyManagerUtilities();
  ~vtkSMProxyManagerUtilities();

private:
  vtkSMProxyManagerUtilities(const vtkSMProxyManagerUtilities&) = delete;
  void operator=(const vtkSMProxyManagerUtilities&) = delete;

  vtkSMSessionProxyManager* ProxyManager;
};

#endif
