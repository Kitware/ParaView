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
// .NAME vtkSMSelectionLink -
// .SECTION Description
// Creates a link between two properties. Can create M->N links.
// At the time when the link is created every output Selection is synchronized
// with the first input Selection.

#ifndef vtkSMSelectionLink_h
#define vtkSMSelectionLink_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMLink.h"

//BTX
class vtkSMSourceProxy;
class vtkSMSelectionLinkInternals;
class vtkSMSelectionLinkObserver;
//ETX

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSelectionLink : public vtkSMLink
{
public:
  static vtkSMSelectionLink* New();
  vtkTypeMacro(vtkSMSelectionLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a selection to the link. updateDir determines whether
  // the proxy used is an input or an output. When a selection of an input proxy
  // changes, it's selection is set to all other output proxies in the link.
  // A selection can be set to be both input and output by adding 2 links, one
  // to INPUT and the other to OUTPUT
  // When a link is added, all output Selection values are
  // synchronized with that of the input.
  void AddLinkedSelection(vtkSMProxy* proxy,
                          int updateDir);
  void RemoveLinkedSelection(vtkSMProxy* proxy);

  // Description:
  // Get the number of properties that are involved in this link.
  unsigned int GetNumberOfLinkedObjects();

  // Description:
  // Get a proxy involved in this link.
  vtkSMProxy* GetLinkedProxy(int index);

  // Description:
  // Get the direction of a Selection involved in this link
  // (see vtkSMLink::UpdateDirections)
  int GetLinkedObjectDirection(int index);

  // Description:
  // Remove all links.
  virtual void RemoveAllLinks();

  // Description:
  // This method is used to initialize the object to the given protobuf state
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator);

protected:
  vtkSMSelectionLink();
  ~vtkSMSelectionLink();

  friend class vtkSMSelectionLinkInternals;
  friend class vtkSMSelectionLinkObserver;

  // Description:
  // Load the link state.
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator);

  // Description:
  // Save the state of the link.
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Not implemented
  virtual void UpdateVTKObjects(vtkSMProxy* vtkNotUsed(caller)){};

  // Description:
  // Not implemented
  virtual void PropertyModified(vtkSMProxy* vtkNotUsed(caller), const char* vtkNotUsed(pname)){};

  // Description:
  // Not implemented
  virtual void UpdateProperty(vtkSMProxy* vtkNotUsed(caller), const char* vtkNotUsed(pname)){};

  // Description:
  // This method find the caller in the link and update selection output accordingly
  virtual void SelectionModified(vtkSMSourceProxy* caller, unsigned int portIndex);

  // Description:
  // Update the internal protobuf state
  virtual void UpdateState();

private:
  vtkSMSelectionLinkInternals* Internals;

  // lock flag to prevent multiple selection modification at the same time
  bool ModifyingSelection;

  vtkSMSelectionLink(const vtkSMSelectionLink&); // Not implemented.
  void operator=(const vtkSMSelectionLink&); // Not implemented.
};
#endif
