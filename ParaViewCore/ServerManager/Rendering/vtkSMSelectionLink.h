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
/**
 * @class   vtkSMSelectionLink
 *
 * Creates a link between two properties. Can create M->N links.
 * At the time when the link is created every output Selection is synchronized
 * with the first input Selection.
*/

#ifndef vtkSMSelectionLink_h
#define vtkSMSelectionLink_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMLink.h"

class vtkSMSourceProxy;
class vtkSMSelectionLinkInternals;
class vtkSMSelectionLinkObserver;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMSelectionLink : public vtkSMLink
{
public:
  static vtkSMSelectionLink* New();
  vtkTypeMacro(vtkSMSelectionLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Add a selection to the link. updateDir determines whether
   * the proxy used is an input or an output. When a selection of an input proxy
   * changes, it's selection is set to all other output proxies in the link.
   * A selection can be set to be both input and output by adding 2 links, one
   * to INPUT and the other to OUTPUT
   * When a link is added, all output Selection values are
   * synchronized with that of the input.
   */
  void AddLinkedSelection(vtkSMProxy* proxy, int updateDir);
  void RemoveLinkedSelection(vtkSMProxy* proxy);
  //@}

  /**
   * Get the number of properties that are involved in this link.
   */
  unsigned int GetNumberOfLinkedObjects() VTK_OVERRIDE;

  /**
   * Get a proxy involved in this link.
   */
  vtkSMProxy* GetLinkedProxy(int index) VTK_OVERRIDE;

  /**
   * Get the direction of a Selection involved in this link
   * (see vtkSMLink::UpdateDirections)
   */
  int GetLinkedObjectDirection(int index) VTK_OVERRIDE;

  /**
   * Remove all links.
   */
  virtual void RemoveAllLinks() VTK_OVERRIDE;

  /**
   * This method is used to initialize the object to the given protobuf state
   */
  virtual void LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Set/Get the convert to indices flag. When on, the input selection
   * will always be converted into an indices based selection
   * before being applied to outputs
   */
  vtkSetMacro(ConvertToIndices, bool);
  vtkGetMacro(ConvertToIndices, bool);

protected:
  vtkSMSelectionLink();
  ~vtkSMSelectionLink();

  friend class vtkSMSelectionLinkInternals;
  friend class vtkSMSelectionLinkObserver;

  /**
   * Load the link state.
   */
  virtual int LoadXMLState(vtkPVXMLElement* linkElement, vtkSMProxyLocator* locator) VTK_OVERRIDE;

  /**
   * Save the state of the link.
   */
  virtual void SaveXMLState(const char* linkname, vtkPVXMLElement* parent) VTK_OVERRIDE;

  /**
   * Not implemented
   */
  virtual void UpdateVTKObjects(vtkSMProxy* vtkNotUsed(caller)) VTK_OVERRIDE{};

  /**
   * Not implemented
   */
  virtual void PropertyModified(
    vtkSMProxy* vtkNotUsed(caller), const char* vtkNotUsed(pname)) VTK_OVERRIDE{};

  /**
   * Not implemented
   */
  virtual void UpdateProperty(
    vtkSMProxy* vtkNotUsed(caller), const char* vtkNotUsed(pname)) VTK_OVERRIDE{};

  /**
   * This method find the caller in the link and update selection output accordingly
   */
  virtual void SelectionModified(vtkSMSourceProxy* caller, unsigned int portIndex);

  /**
   * Update the internal protobuf state
   */
  virtual void UpdateState() VTK_OVERRIDE;

private:
  vtkSMSelectionLinkInternals* Internals;

  // lock flag to prevent multiple selection modification at the same time
  bool ModifyingSelection;
  bool ConvertToIndices;

  vtkSMSelectionLink(const vtkSMSelectionLink&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMSelectionLink&) VTK_DELETE_FUNCTION;
};
#endif
