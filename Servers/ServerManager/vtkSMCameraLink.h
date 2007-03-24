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
class vtkSMRenderModuleProxy;

//BTX
struct vtkSMCameraLinkInternals;
//ETX

class VTK_EXPORT vtkSMCameraLink : public vtkSMProxyLink
{
public:
  static vtkSMCameraLink* New();
  vtkTypeRevisionMacro(vtkSMCameraLink, vtkSMProxyLink);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  void StartInteraction(vtkObject* caller);
  void EndInteraction(vtkObject* caller);

protected:
  vtkSMCameraLink();
  ~vtkSMCameraLink();

  virtual void UpdateProperties(vtkSMProxy* caller, const char* pname);
  virtual void UpdateVTKObjects(vtkSMProxy* caller);

  // Description:
  // Save the state of the link.
  virtual void SaveState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Load the link state.
  virtual int LoadState(vtkPVXMLElement* linkElement, vtkSMStateLoader* loader);
private:

  vtkSMCameraLinkInternals* Internals;

  vtkSMCameraLink(const vtkSMCameraLink&); // Not implemented
  void operator=(const vtkSMCameraLink&); // Not implemented
};

#endif
