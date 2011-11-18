/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyLink - creates a link between two proxies.
// .SECTION Description
// When a link is created between proxy A->B, whenever any property
// on proxy A is modified, a property with the same name as the modified
// property (if any) on proxy B is also modified to be the same as the property
// on the proxy A. Similary whenever proxy A->UpdateVTKObjects() is called,
// B->UpdateVTKObjects() is also fired.

#ifndef __vtkSMProxyLink_h
#define __vtkSMProxyLink_h

#include "vtkSMLink.h"

//BTX
struct vtkSMProxyLinkInternals;
//ETX

class VTK_EXPORT vtkSMProxyLink : public vtkSMLink
{
public:
  static vtkSMProxyLink* New();
  vtkTypeMacro(vtkSMProxyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  // A proxy can be set to be both input and output by adding 2 link, one
  // to INPUT and the other to OUTPUT
  virtual void AddLinkedProxy(vtkSMProxy* proxy, int updateDir);

  // Description:
  // Remove a linked proxy.
  virtual void RemoveLinkedProxy(vtkSMProxy* proxy);

  // Description:
  // Get the number of proxies that are involved in this link.
  unsigned int GetNumberOfLinkedProxies();

  // Description:
  // Get a proxy involved in this link.
  vtkSMProxy* GetLinkedProxy(int index);
  
  // Description:
  // Get the direction of a proxy involved in this link
  // (see vtkSMLink::UpdateDirections)
  int GetLinkedProxyDirection(int index);
  
  // Description:
  // It is possible to exclude certain properties from being synchronized
  // by this link. This method can be used to add/remove the names for such
  // exception properties.
  void AddException(const char* propertyname);
  void RemoveException(const char* propertyname);

  // Description:
  // Remove all links.
  virtual void RemoveAllLinks();

//BTX

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMProxyLink();
  ~vtkSMProxyLink();

  // Description:
  // Called when an input proxy is updated (UpdateVTKObjects). 
  // Argument is the input proxy.
  virtual void UpdateVTKObjects(vtkSMProxy* proxy);

  // Description:
  // Called when a property of an input proxy is modified.
  // caller:- the input proxy.
  // pname:- name of the property being modified.
  virtual void PropertyModified(vtkSMProxy* proxy, const char* pname);

  // Description:
  // Called when a property is pushed.
  // caller :- the input proxy.
  // pname :- name of property that was pushed.
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname);

  // Description:
  // Save the state of the link.
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Load the link state.
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator);

  // Description:
  // Update the internal protobuf state
  virtual void UpdateState();

private:
  vtkSMProxyLinkInternals* Internals;

  vtkSMProxyLink(const vtkSMProxyLink&); // Not implemented
  void operator=(const vtkSMProxyLink&); // Not implemented
//ETX
};

#endif
