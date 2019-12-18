/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMLink
 * @brief Abstract base class for proxy/property links.
 *
 * Abstract base class for proxy/property links. Links provide a means
 * to connect two properties (or proxies) together, thus when one is updated,
 * the dependent one is updated accordingly.
 */

#ifndef vtkSMLink_h
#define vtkSMLink_h

#include "vtkRemotingServerManagerModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"            // Needed
#include "vtkSMRemoteObject.h"

class vtkCommand;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkSMLink : public vtkSMRemoteObject
{
public:
  vtkTypeMacro(vtkSMLink, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum UpdateDirections
  {
    NONE = 0,
    INPUT = 1,
    OUTPUT = 2
  };

  //@{
  /**
   * This flag determines if UpdateVTKObjects calls are to be propagated.
   * Set to 1 by default.
   */
  vtkSetMacro(PropagateUpdateVTKObjects, int);
  vtkGetMacro(PropagateUpdateVTKObjects, int);
  vtkBooleanMacro(PropagateUpdateVTKObjects, int);
  //@}

  //@{
  /**
   * Get/Set if the link is enabled.
   * (true by default).
   */
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  //@}

  /**
   * Remove all links.
   */
  virtual void RemoveAllLinks() = 0;

  /**
   * This method returns the full object state that can be used to create the
   * object from scratch.
   * This method will be used to fill the undo stack.
   * If not overridden this will return NULL.
   */
  const vtkSMMessage* GetFullState() override;

  /**
   * This method is used to initialize the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalIDs set. This enables splitting the load process in 2 step to prevent
   * invalid state when a property refers to a sub-proxy that does not exist yet.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

  /**
   * Update the internal protobuf state
   */
  virtual void UpdateState() = 0;

  /**
   * Get the number of object that are involved in this link.
   */
  virtual unsigned int GetNumberOfLinkedObjects() = 0;

  /**
   * Get the direction of a object involved in this link
   * (see vtkSMLink::UpdateDirections)
   */
  virtual int GetLinkedObjectDirection(int index) = 0;

  /**
   * Get a proxy involved in this link.
   */
  virtual vtkSMProxy* GetLinkedProxy(int index) = 0;

protected:
  vtkSMLink();
  ~vtkSMLink() override;

  /**
   * When the state has changed we call that method so the state can be shared
   * if any collaboration is involved.
   */
  void PushStateToSession();

  /**
   * Called when an input proxy is updated (UpdateVTKObjects).
   * Argument is the input proxy.
   */
  virtual void UpdateVTKObjects(vtkSMProxy* proxy) = 0;

  /**
   * Called when a property of an input proxy is modified.
   * caller:- the input proxy.
   * pname:- name of the property being modified.
   */
  virtual void PropertyModified(vtkSMProxy* proxy, const char* pname) = 0;

  /**
   * Called when a property is pushed.
   * caller :- the input proxy.
   * pname :- name of property that was pushed.
   */
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname) = 0;

  /**
   * Subclasses call this method to observer events on a INPUT proxy.
   */
  void ObserveProxyUpdates(vtkSMProxy* proxy);

  /**
   * Save the state of the link.
   */
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) = 0;

  /**
   * Load the link state.
   */
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator) = 0;

  friend class vtkSMLinkObserver;
  friend class vtkSMStateLoader;
  friend class vtkSMSessionProxyManager;

  vtkCommand* Observer;
  // Set by default. In a link P1->P2, if this flag is set, when ever Proxy with P1
  // is updated i.e. UpdateVTKObjects() is called, this class calls
  // UpdateVTKObjects on Proxy with P2.
  int PropagateUpdateVTKObjects;

  bool Enabled;

  // Cached version of State
  vtkSMMessage* State;

private:
  vtkSMLink(const vtkSMLink&) = delete;
  void operator=(const vtkSMLink&) = delete;
};

#endif
