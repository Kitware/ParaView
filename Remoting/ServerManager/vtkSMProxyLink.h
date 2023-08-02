// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMProxyLink
 * @brief   creates a link between two proxies.
 *
 * When a link is created between proxy A->B, whenever any property
 * on proxy A is modified, a property with the same name as the modified
 * property (if any) on proxy B is also modified to be the same as the property
 * on the proxy A. Similarly whenever proxy A->UpdateVTKObjects() is called,
 * B->UpdateVTKObjects() is also fired.
 */

#ifndef vtkSMProxyLink_h
#define vtkSMProxyLink_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMLink.h"

#include <memory> // for unique_ptr

struct vtkSMProxyLinkInternals;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMProxyLink : public vtkSMLink
{
public:
  static vtkSMProxyLink* New();
  vtkTypeMacro(vtkSMProxyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ExceptionListBehavior : int
  {
    BLACKLIST = 0,
    WHITELIST
  };

  /**
   * Add a property to the link. updateDir determines whether a property of
   * the proxy is read or written. When a property of an input proxy
   * changes, it's value is pushed to all other output proxies in the link.
   * A proxy can be set to be both input and output by adding 2 link, one
   * to INPUT and the other to OUTPUT
   */
  virtual void AddLinkedProxy(vtkSMProxy* proxy, int updateDir);

  /**
   * Utility method to add 2 proxies, both as INPUT and OUTPUT.
   * This is equivalent to call 4 times AddLinkedProxy() with each combination.
   */
  virtual void LinkProxies(vtkSMProxy* proxy1, vtkSMProxy* proxy2);

  /**
   * Utility method to link the proxies stored as proxy property of input proxies.
   */
  virtual void LinkProxyPropertyProxies(vtkSMProxy* proxy1, vtkSMProxy* proxy2, const char* pname);

  /**
   * Remove a linked proxy.
   */
  virtual void RemoveLinkedProxy(vtkSMProxy* proxy);

  ///@{
  /**
   * Get the number of proxies that are involved in this link.
   */
  unsigned int GetNumberOfLinkedObjects() override;
  unsigned int GetNumberOfLinkedProxies();
  ///@}

  /**
   * Get a proxy involved in this link.
   */
  vtkSMProxy* GetLinkedProxy(int index) override;

  ///@{
  /**
   * Get the direction of a proxy involved in this link
   * (see vtkSMLink::UpdateDirections)
   */
  int GetLinkedObjectDirection(int index) override;
  int GetLinkedProxyDirection(int index);
  ///@}

  ///@{
  /**
   * It is possible to exclude certain properties from being synchronized
   * by this link. This method can be used to add/remove the names for such
   * exception properties.
   * If ExceptionBehavior is set to BLACKLIST (default), exceptions are excluded
   * from synchronization.
   * If ExceptionBehavior is set to WHITELIST, exceptions are the only one
   * synchronized.
   */
  void AddException(const char* propertyname);
  void RemoveException(const char* propertyname);
  void ClearExceptions();
  ///@}

  /**
   * Remove all links.
   */
  void RemoveAllLinks() override;

  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

  ///@{
  /**
   * Set/Get exception behavior. The list can be a blacklist or a whitelist
   * of proxy properties. Properties in the list will not be linked if
   * behavior is set to BLACKLIST, or they will be the only ones linked if
   * behavior is set to WHITELIST.
   * Default: BLACKLIST.
   */
  vtkSetMacro(ExceptionBehavior, int);
  vtkGetMacro(ExceptionBehavior, int);
  void SetExceptionBehaviorToBlacklist() { this->SetExceptionBehavior(BLACKLIST); }
  void SetExceptionBehaviorToWhitelist() { this->SetExceptionBehavior(WHITELIST); }
  ///@}

protected:
  vtkSMProxyLink();
  ~vtkSMProxyLink() override;

  /**
   * Called when an input proxy is updated (UpdateVTKObjects).
   * Argument is the input proxy.
   */
  void UpdateVTKObjects(vtkSMProxy* proxy) override;

  /**
   * Called when a property of an input proxy is modified.
   * caller:- the input proxy.
   * pname:- name of the property being modified.
   */
  void PropertyModified(vtkSMProxy* proxy, const char* pname) override;

  /**
   * Called when a property is pushed.
   * caller :- the input proxy.
   * pname :- name of property that was pushed.
   */
  void UpdateProperty(vtkSMProxy* caller, const char* pname) override;

  /**
   * Get tag name to use in statefile. This should match
   * the class name without "vtkSM" prefix.
   * see vtkSMStateLoader::HandleLinks
   */
  virtual std::string GetXMLTagName() { return "ProxyLink"; }

  /**
   * Save the state of the link.
   */
  void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) override;

  /**
   * Load the link state.
   */
  int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator) override;

  /**
   * Update the internal protobuf state
   */
  void UpdateState() override;

private:
  vtkSMProxyLink(const vtkSMProxyLink&) = delete;
  void operator=(const vtkSMProxyLink&) = delete;

  /**
   * Utility function to know whether a property is linked or not.
   * Checks in exception list, depending on exception behavior.
   */
  bool isPropertyLinked(const char* pname);

  std::unique_ptr<vtkSMProxyLinkInternals> Internals;
  int ExceptionBehavior = BLACKLIST;
};

#endif
