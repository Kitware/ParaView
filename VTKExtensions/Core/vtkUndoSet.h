/*=========================================================================

  Program:   ParaView
  Module:    vtkUndoSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUndoSet
 * @brief   Maintains a collection of vtkUndoElement that can be
 * undone/redone in a single step.
 *
 * This is a concrete class that stores a collection of vtkUndoElement objects.
 * A vtkUndoSet object represents an atomic undo-redoable operation. It can
 * contain one or more vtkUndoElement objects. When added vtkUndoElement objects
 * to a vtkUndoSet they must be added in the sequence of operation. When undoing
 * the operations are performed in reverse order, while when redoing they are
 * performed in forward order.
 *
 * vtkUndoElement, vtkUndoSet and vtkUndoStack form the undo/redo framework core.
 * @sa
 * vtkUndoStack vtkUndoElement
*/

#ifndef vtkUndoSet_h
#define vtkUndoSet_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

class vtkCollection;
class vtkPVXMLElement;
class vtkUndoElement;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkUndoSet : public vtkObject
{
public:
  static vtkUndoSet* New();
  vtkTypeMacro(vtkUndoSet, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform an Undo.
   */
  virtual int Undo();

  /**
   * Perform a Redo.
   */
  virtual int Redo();

  /**
   * Add an element to this set. If the newly added element, \c elem, and
   * the most recently added element are both \c Mergeable, then an
   * attempt is made to merge the new element with the previous one. On
   * successful merging, the new element is discarded, otherwise
   * it is appended to the set.
   * \returns the index at which the element got added/merged.
   */
  int AddElement(vtkUndoElement* elem);

  /**
   * Remove an element at a particular index.
   */
  void RemoveElement(int index);

  /**
   * Get an element at a particular index
   */
  vtkUndoElement* GetElement(int index);

  /**
   * Remove all elemments.
   */
  void RemoveAllElements();

  /**
   * Get number of elements in the set.
   */
  int GetNumberOfElements();

protected:
  vtkUndoSet();
  ~vtkUndoSet() override;

  vtkCollection* Collection;
  vtkCollection* TmpWorkingCollection;

private:
  vtkUndoSet(const vtkUndoSet&) = delete;
  void operator=(const vtkUndoSet&) = delete;
};

#endif
