/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCameraLink
 * @brief   creates a link between two cameras.
 *
 * When a link is created between camera A->B, whenever any property
 * on camera A is modified, a property with the same name as the modified
 * property (if any) on camera B is also modified to be the same as the property
 * on the camera A. Similarly whenever camera A->UpdateVTKObjects() is called,
 * B->UpdateVTKObjects() is also fired.
*/

#ifndef vtkSMCameraLink_h
#define vtkSMCameraLink_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxyLink.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMCameraLink : public vtkSMProxyLink
{
public:
  static vtkSMCameraLink* New();
  vtkTypeMacro(vtkSMCameraLink, vtkSMProxyLink);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set if the link should synchronize interactive renders
   * as well. On by default.
   */
  vtkSetMacro(SynchronizeInteractiveRenders, int);
  vtkGetMacro(SynchronizeInteractiveRenders, int);
  vtkBooleanMacro(SynchronizeInteractiveRenders, int);
  //@}

  /**
   * Add a property to the link. updateDir determines whether a property of
   * the proxy is read or written. When a property of an input proxy
   * changes, it's value is pushed to all other output proxies in the link.
   * A proxy can be set to be both input and output by setting updateDir
   * to INPUT | OUTPUT
   */
  void AddLinkedProxy(vtkSMProxy* proxy, int updateDir) override;

  /**
   * Remove a linked proxy.
   */
  void RemoveLinkedProxy(vtkSMProxy* proxy) override;

  /**
   * Update all the views linked with an OUTPUT direction.
   * \c interactive indicates if the render is interactive or not.
   */
  virtual void UpdateViews(vtkSMProxy* caller, bool interactive);

  /**
   * This method is used to initialise the object to the given state
   * If the definitionOnly Flag is set to True the proxy won't load the
   * properties values and just setup the new proxy hierarchy with all subproxy
   * globalID set. This allow to split the load process in 2 step to prevent
   * invalid state when property refere to a sub-proxy that does not exist yet.
   */
  void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) override;

protected:
  vtkSMCameraLink();
  ~vtkSMCameraLink() override;

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
  void UpdateProperty(vtkSMProxy*, const char*) override {}

  /**
   * Save the state of the link.
   */
  void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) override;

  /**
   * Internal method to copy vtkSMproperty values from caller to all linked
   * proxies.
   */
  void CopyProperties(vtkSMProxy* caller);

  void ResetCamera(vtkObject* caller);

  int SynchronizeInteractiveRenders;

  /**
   * Update the internal protobuf state
   */
  void UpdateState() override;

private:
  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;

  vtkSMCameraLink(const vtkSMCameraLink&) = delete;
  void operator=(const vtkSMCameraLink&) = delete;
};

#endif
