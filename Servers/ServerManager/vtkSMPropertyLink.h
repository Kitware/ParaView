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

#ifndef __vtkSMPropertyLink_h
#define __vtkSMPropertyLink_h

#include "vtkSMLink.h"

//BTX
class vtkSMProperty;
struct vtkSMPropertyLinkInternals;
//ETX

class VTK_EXPORT vtkSMPropertyLink : public vtkSMLink
{
public:
  static vtkSMPropertyLink* New();
  vtkTypeRevisionMacro(vtkSMPropertyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  void AddLinkedProperty(vtkSMProxy* proxy, const char* propertyname, int updateDir);

 
protected:
  vtkSMPropertyLink();
  ~vtkSMPropertyLink();

//BTX
  friend struct vtkSMPropertyLinkInternals;
//ETX

  // Description:
  // Load the link state.
  virtual int LoadState(vtkPVXMLElement* linkElement, vtkSMStateLoader* loader);

  // Description:
  // Save the state of the link.
  virtual void SaveState(const char* linkname, vtkPVXMLElement* parent);
  
  virtual void UpdateVTKObjects(vtkSMProxy* caller);
  virtual void UpdateProperties(vtkSMProxy* caller, const char* pname);
private:
  vtkSMPropertyLinkInternals* Internals;

  vtkSMPropertyLink(const vtkSMPropertyLink&); // Not implemented.
  void operator=(const vtkSMPropertyLink&); // Not implemented.
};


#endif

