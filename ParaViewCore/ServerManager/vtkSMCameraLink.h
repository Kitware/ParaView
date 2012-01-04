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
// .NAME vtkSMCameraLink - creates a link between two cameras.
// .SECTION Description
// When a link is created between camera A->B, whenever any property
// on camera A is modified, a property with the same name as the modified
// property (if any) on camera B is also modified to be the same as the property
// on the camera A. Similary whenever camera A->UpdateVTKObjects() is called,
// B->UpdateVTKObjects() is also fired.

#ifndef __vtkSMCameraLink_h
#define __vtkSMCameraLink_h

#include "vtkSMProxyLink.h"

class VTK_EXPORT vtkSMCameraLink : public vtkSMProxyLink
{
public:
  static vtkSMCameraLink* New();
  vtkTypeMacro(vtkSMCameraLink, vtkSMProxyLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set if the link should synchronize interactive renders
  // as well. On by default.
  vtkSetMacro(SynchronizeInteractiveRenders, int);
  vtkGetMacro(SynchronizeInteractiveRenders, int);
  vtkBooleanMacro(SynchronizeInteractiveRenders, int);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  // A proxy can be set to be both input and output by setting updateDir
  // to INPUT | OUTPUT
  void AddLinkedProxy(vtkSMProxy* proxy, int updateDir);
  
  // Description:
  // Remove a linked proxy.
  virtual void RemoveLinkedProxy(vtkSMProxy* proxy);
  
  // Description:
  // Update all the views linked with an OUTPUT direction.
  // \c interactive indicates if the render is interactive or not.
  virtual void UpdateViews(vtkSMProxy* caller, bool interactive);
//BTX

  // Description:
  // This method is used to initialise the object to the given state
  // If the definitionOnly Flag is set to True the proxy won't load the
  // properties values and just setup the new proxy hierarchy with all subproxy
  // globalID set. This allow to split the load process in 2 step to prevent
  // invalid state when property refere to a sub-proxy that does not exist yet.
  virtual void LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMCameraLink();
  ~vtkSMCameraLink();

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
  virtual void UpdateProperty(vtkSMProxy* , const char* ) {}

  // Description:
  // Save the state of the link.
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Internal method to copy vtkSMproperty values from caller to all linked
  // proxies.
  void CopyProperties(vtkSMProxy* caller);

  void StartInteraction(vtkObject* caller);
  void EndInteraction(vtkObject* caller);
  void ResetCamera(vtkObject* caller);

  int SynchronizeInteractiveRenders;

  // Description:
  // Update the internal protobuf state
  virtual void UpdateState();

private:

  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;

  vtkSMCameraLink(const vtkSMCameraLink&); // Not implemented
  void operator=(const vtkSMCameraLink&); // Not implemented
//ETX
};

#endif
