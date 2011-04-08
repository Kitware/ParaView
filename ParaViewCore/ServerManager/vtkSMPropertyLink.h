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

#include "vtkSMLink.h"

//BTX
class vtkSMProperty;
struct vtkSMPropertyLinkInternals;
class vtkSMPropertyLinkObserver;
//ETX

class VTK_EXPORT vtkSMPropertyLink : public vtkSMLink
{
public:
  static vtkSMPropertyLink* New();
  vtkTypeMacro(vtkSMPropertyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  // A property can be set to be both input and output by setting updateDir
  // to INPUT | OUTPUT.
  // When a link is added, all output property values are
  // synchronized with that of the input.
  void AddLinkedProperty(vtkSMProxy* proxy, 
                         const char* propertyname, 
                         int updateDir);
  void RemoveLinkedProperty(vtkSMProxy* proxy, const char* propertyname);

  // Description:
  // Another API to add a property link. In this case. we don't provide
  // the link with the proxy to which the property belongs. Consequently
  // we cannot propagate UpdateVTKObjects() calls irrespective
  // of the PropagateUpdateVTKObjects flag. If one wants to propagate 
  // UpdateVTKObjects, use the overload with vtkSMProxy as the argument.
  // When a link is added, all output property values are
  // synchronized with that of the input.
  void AddLinkedProperty(vtkSMProperty* property, int updateDir);

  // Description:
  // Remove a linked property.
  void RemoveLinkedProperty(vtkSMProperty* property);

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
protected:
  vtkSMPropertyLink();
  ~vtkSMPropertyLink();


  // Description:
  // Synchornize the value of all output properties
  // with the input property.
  void Synchronize();
//BTX
  friend struct vtkSMPropertyLinkInternals;
  friend class vtkSMPropertyLinkObserver;
//ETX

  // Description:
  // Load the link state.
  virtual int LoadState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator);

  // Description:
  // Save the state of the link.
  virtual void SaveState(const char* linkname, vtkPVXMLElement* parent);
  
  virtual void UpdateVTKObjects(vtkSMProxy* caller);
  virtual void PropertyModified(vtkSMProxy* caller, const char* pname);
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname);
  void PropertyModified(vtkSMProperty* property);
private:
  vtkSMPropertyLinkInternals* Internals;
  bool ModifyingProperty;

  vtkSMPropertyLink(const vtkSMPropertyLink&); // Not implemented.
  void operator=(const vtkSMPropertyLink&); // Not implemented.
};


#endif

