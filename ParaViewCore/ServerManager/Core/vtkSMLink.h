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
// .NAME vtkSMLink - Abstract base class for proxy/property links.
// .SECTION Description
// Abstract base class for proxy/property links. Links provide a means
// to connect two properies(or proxies) together, thus when one is updated,
// the dependent one is also updated accordingly.

#ifndef __vtkSMLink_h
#define __vtkSMLink_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMRemoteObject.h"
#include "vtkSMMessageMinimal.h" // Needed
//BTX
class vtkCommand;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMProxyLocator;
//ETX

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMLink : public vtkSMRemoteObject
{
public:
  vtkTypeMacro(vtkSMLink, vtkSMRemoteObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum UpdateDirections
    {
    NONE = 0,
    INPUT = 1,
    OUTPUT = 2
    };
//ETX

  // Description:
  // This flag determines if UpdateVTKObjects calls are to be propagated.
  // Set to 1 by default.
  vtkSetMacro(PropagateUpdateVTKObjects, int);
  vtkGetMacro(PropagateUpdateVTKObjects, int);
  vtkBooleanMacro(PropagateUpdateVTKObjects, int);

  // Description:
  // Get/Set if the link is enabled.
  // (true by default).
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);

  // Description:
  // Remove all links.
  virtual void RemoveAllLinks() = 0;

//BTX

  // Description:
  // This method returns the full object state that can be used to create the
  // object from scratch.
  // This method will be used to fill the undo stack.
  // If not overriden this will return NULL.
  virtual const vtkSMMessage* GetFullState();

  // Description:
  // This method is used to initialize the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalIDs set. This enables splitting the load process in 2 step to prevent
  // invalid state when a property refers to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMLink();
  ~vtkSMLink();

  // Description:
  // When the state has changed we call that method so the state can be shared
  // if any collaboration is involved.
  void PushStateToSession();

  // Description:
  // Called when an input proxy is updated (UpdateVTKObjects). 
  // Argument is the input proxy.
  virtual void UpdateVTKObjects(vtkSMProxy* proxy)=0;

  // Description:
  // Called when a property of an input proxy is modified.
  // caller:- the input proxy.
  // pname:- name of the property being modified.
  virtual void PropertyModified(vtkSMProxy* proxy, const char* pname)=0;

  // Description:
  // Called when a property is pushed.
  // caller :- the input proxy.
  // pname :- name of property that was pushed.
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname)=0;

  // Description:
  // Subclasses call this method to observer events on a INPUT proxy.
  void ObserveProxyUpdates(vtkSMProxy* proxy);

  // Description:
  // Save the state of the link.
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) = 0;

  // Description:
  // Load the link state.
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator) = 0;

  friend class vtkSMLinkObserver;
  friend class vtkSMStateLoader;
  friend class vtkSMSessionProxyManager;
//ETX
  vtkCommand* Observer;
  // Set by default. In a link P1->P2, if this flag is set, when ever Proxy with P1
  // is updated i.e. UpdateVTKObjects() is called, this class calls
  // UpdateVTKObjects on Proxy with P2.
  int PropagateUpdateVTKObjects;

  bool Enabled;

  // Cached version of State
  vtkSMMessage* State;
private:
  vtkSMLink(const vtkSMLink&); // Not implemented.
  void operator=(const vtkSMLink&); // Not implemented.

//ETX
};

#endif

