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
/**
 * @class   vtkSMPropertyLink
 *
 * Creates a link between two properties. Can create M->N links.
 * At the time when the link is created every output property is synchornized
 * with the first input property.
*/

#ifndef vtkSMPropertyLink_h
#define vtkSMPropertyLink_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMLink.h"

class vtkSMProperty;
class vtkSMPropertyLinkInternals;
class vtkSMPropertyLinkObserver;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMPropertyLink : public vtkSMLink
{
public:
  static vtkSMPropertyLink* New();
  vtkTypeMacro(vtkSMPropertyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Add a property to the link. updateDir determines whether a property of
   * the proxy is read or written. When a property of an input proxy
   * changes, it's value is pushed to all other output proxies in the link.
   * A property can be set to be both input and output by adding 2 links, one
   * to INPUT and the other to OUTPUT
   * When a link is added, all output property values are
   * synchronized with that of the input.
   */
  void AddLinkedProperty(vtkSMProxy* proxy, const char* propertyname, int updateDir);
  void RemoveLinkedProperty(vtkSMProxy* proxy, const char* propertyname);
  //@}

  //@{
  /**
   * Get the number of properties that are involved in this link.
   */
  unsigned int GetNumberOfLinkedObjects() VTK_OVERRIDE;
  unsigned int GetNumberOfLinkedProperties();
  //@}

  /**
   * Get a property involved in this link.
   */
  vtkSMProperty* GetLinkedProperty(int index);

  /**
   * Get a proxy involved in this link.
   */
  vtkSMProxy* GetLinkedProxy(int index) VTK_OVERRIDE;

  /**
   * Get a property involved in this link.
   */
  const char* GetLinkedPropertyName(int index);

  //@{
  /**
   * Get the direction of a property involved in this link
   * (see vtkSMLink::UpdateDirections)
   */
  int GetLinkedObjectDirection(int index) VTK_OVERRIDE;
  int GetLinkedPropertyDirection(int index);
  //@}

  /**
   * Remove all links.
   */
  virtual void RemoveAllLinks() VTK_OVERRIDE;

  /**
   * This method is used to initialize the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalIDs set. This enables splitting the load process in 2 step to prevent
   * invalid state when a property refers to a sub-proxy that does not exist yet.
   */
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) VTK_OVERRIDE;

protected:
  vtkSMPropertyLink();
  ~vtkSMPropertyLink();

  /**
   * Synchronize the value of all output properties with the input property.
   */
  void Synchronize();

  friend class vtkSMPropertyLinkInternals;
  friend class vtkSMPropertyLinkObserver;

  /**
   * Load the link state.
   */
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Save the state of the link.
   */
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) VTK_OVERRIDE;

  virtual void UpdateVTKObjects(vtkSMProxy* caller) VTK_OVERRIDE;
  virtual void PropertyModified(vtkSMProxy* caller, const char* pname) VTK_OVERRIDE;
  virtual void PropertyModified(vtkSMProperty* property);
  virtual void UpdateProperty(vtkSMProxy* caller, const char* pname) VTK_OVERRIDE;

  /**
   * Update the internal protobuf state
   */
  virtual void UpdateState() VTK_OVERRIDE;

private:
  vtkSMPropertyLinkInternals* Internals;
  bool ModifyingProperty;

  vtkSMPropertyLink(const vtkSMPropertyLink&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPropertyLink&) VTK_DELETE_FUNCTION;
};

#endif
