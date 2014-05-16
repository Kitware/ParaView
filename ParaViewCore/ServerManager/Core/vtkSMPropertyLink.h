/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPropertyLink -
// .SECTION Description
// Creates a link between two properties. Can create M->N links.
// At the time when the link is created every output property is synchornized 
// with the first input property.

#ifndef __vtkSMPropertyLink_h
#define __vtkSMPropertyLink_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMLink.h"

//BTX
class vtkSMProperty;
class vtkSMPropertyLinkInternals;
class vtkSMPropertyLinkObserver;
//ETX

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMPropertyLink : public vtkSMLink
{
public:
  static vtkSMPropertyLink* New();
  vtkTypeMacro(vtkSMPropertyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  // A property can be set to be both input and output by adding 2 links, one
  // to INPUT and the other to OUTPUT
  // When a link is added, all output property values are
  // synchronized with that of the input.
  void AddLinkedProperty(vtkSMProxy* proxy, 
                         const char* propertyname, 
                         int updateDir);
  void RemoveLinkedProperty(vtkSMProxy* proxy, const char* propertyname);

  // Description:
  // Get the number of properties that are involved in this link.
  unsigned int GetNumberOfLinkedProperties();

  // Description:
  // Get a property involved in this link.
  vtkSMProperty* GetLinkedProperty(int index);

  // Description:
  // Get a proxy involved in this link.
  vtkSMProxy* GetLinkedProxy(int index);
  
  // Description:
  // Get a property involved in this link.
  const char* GetLinkedPropertyName(int index);
  
  // Description:
  // Get the direction of a property involved in this link
  // (see vtkSMLink::UpdateDirections)
  int GetLinkedPropertyDirection(int index);
  
  // Description:
  // Remove all links.
  virtual void RemoveAllLinks();

//BTX

  // Description:
  // This method is used to initialize the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalIDs set. This enables splitting the load process in 2 step to prevent
  // invalid state when a property refers to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMPropertyLink();
  ~vtkSMPropertyLink();

  // Description:
  // Synchronize the value of all output properties with the input property.
  void Synchronize();

  friend class vtkSMPropertyLinkInternals;
  friend class vtkSMPropertyLinkObserver;

  // Description:
  // Load the link state.
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator);

  // Description:
  // Save the state of the link.
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent);
  
  virtual void UpdateVTKObjects(vtkSMProxy* caller);
  virtual void PropertyModified(vtkSMProxy* caller, const char* pname);
  virtual void PropertyModified(vtkSMProperty* property);
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname);

  // Description:
  // Update the internal protobuf state
  virtual void UpdateState();

private:
  vtkSMPropertyLinkInternals* Internals;
  bool ModifyingProperty;

  vtkSMPropertyLink(const vtkSMPropertyLink&); // Not implemented.
  void operator=(const vtkSMPropertyLink&); // Not implemented.
//ETX
};


#endif

