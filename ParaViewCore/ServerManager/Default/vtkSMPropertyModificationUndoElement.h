/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyModificationUndoElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPropertyModificationUndoElement
 *
 * This is the concrete implementation for the Undo element for a property
 * modification event.
 * The undo action sets the property to the value that was pushed on
 * to the server previous to the modification.
 * The redo action sets the property to the modified value.
*/

#ifndef vtkSMPropertyModificationUndoElement_h
#define vtkSMPropertyModificationUndoElement_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMMessageMinimal.h"             // needed for vtkSMMessage
#include "vtkSMUndoElement.h"
class vtkSMProxy;

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMPropertyModificationUndoElement
  : public vtkSMUndoElement
{
public:
  static vtkSMPropertyModificationUndoElement* New();
  vtkTypeMacro(vtkSMPropertyModificationUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Undo the operation encapsulated by this element.
   */
  int Undo() override;

  /**
   * Redo the operation encaspsulated by this element.
   */
  int Redo() override;

  /**
   * Set the property/proxy that was modified.
   */
  void ModifiedProperty(vtkSMProxy* proxy, const char* propertyname);

  /**
   * Called on the older element in the UndoSet to merge with the
   * element being added if  both the elements are \c mergeable.
   * vtkSMPropertyModificationUndoElement is mergeable with
   * vtkSMPropertyModificationUndoElement alone if both
   * represent change to the same property.
   * Returns if the merge was successful.
   */
  bool Merge(vtkUndoElement* vtkNotUsed(new_element)) override;

protected:
  vtkSMPropertyModificationUndoElement();
  ~vtkSMPropertyModificationUndoElement() override;

  int RevertToState();

  vtkSetStringMacro(PropertyName);

  vtkTypeUInt32 ProxyGlobalID;
  char* PropertyName;
  vtkSMMessage* PropertyState;

private:
  vtkSMPropertyModificationUndoElement(const vtkSMPropertyModificationUndoElement&) = delete;
  void operator=(const vtkSMPropertyModificationUndoElement&) = delete;
};

#endif
