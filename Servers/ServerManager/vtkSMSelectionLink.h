/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionLink.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionLink
// .SECTION Description
// vtkSMSelectionLink links selections between representations. 
// This class simply creates a property link (vtkSMPropertyLink) between
// selection properties of representations.

#ifndef __vtkSMSelectionLink_h
#define __vtkSMSelectionLink_h

#include "vtkSMLink.h"

class vtkSMPropertyLink;

class VTK_EXPORT vtkSMSelectionLink : public vtkSMLink
{
public:
  static vtkSMSelectionLink* New();
  vtkTypeRevisionMacro(vtkSMSelectionLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add linked representation. This API assumes that the name of the property
  // used to set selection is "Selection".
  void AddSelectionLink(vtkSMProxy* repr, int updateDir)
    { this->AddSelectionLink(repr, updateDir, "Selection"); }

  // Description:
  // Add linked representation. This API allows for user to set the name of the
  // property used to set the selection on the representation.
  void AddSelectionLink(vtkSMProxy* repr, int updateDir, const char* pname);


  // Description:
  // Removes a selection link.
  void RemoveSelectionLink(vtkSMProxy* repr)
    { this->RemoveSelectionLink(repr, "Selection"); }
  void RemoveSelectionLink(vtkSMProxy* repr, const char* pname);

  // Description:
  // Remove all links.
  virtual void RemoveAllLinks();
//BTX
protected:
  vtkSMSelectionLink();
  ~vtkSMSelectionLink();
  // Description:
  // Called when an input proxy is updated (UpdateVTKObjects). 
  // Argument is the input proxy.
  virtual void UpdateVTKObjects(vtkSMProxy* ){}

  // Description:
  // Called when a property of an input proxy is modified.
  // caller:- the input proxy.
  // pname:- name of the property being modified.
  virtual void PropertyModified(vtkSMProxy* , const char* ){}

  // Description:
  // Called when a property is pushed.
  // caller :- the input proxy.
  // pname :- name of property that was pushed.
  virtual void UpdateProperty(vtkSMProxy*, const char*) {}

  // Description:
  // Save the state of the link.
  virtual void SaveState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Load the link state.
  virtual int LoadState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator);

  vtkSMPropertyLink* PropertyLink;

private:
  vtkSMSelectionLink(const vtkSMSelectionLink&); // Not implemented
  void operator=(const vtkSMSelectionLink&); // Not implemented
//ETX
};

#endif

