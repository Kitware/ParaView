/*=========================================================================

  Program:   ParaView
  Module:    vtkSessionIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSessionIterator
 *
 * vtkSessionIterator is used to iterate over sessions in the global
 * ProcessModule.
*/

#ifndef vtkSessionIterator_h
#define vtkSessionIterator_h

#include "vtkObject.h"
#include "vtkPVClientServerCoreCoreModule.h" //needed for exports

class vtkSession;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkSessionIterator : public vtkObject
{
public:
  static vtkSessionIterator* New();
  vtkTypeMacro(vtkSessionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Begin iterating over the composite dataset structure.
   */
  virtual void InitTraversal();

  /**
   * Move the iterator to the next item in the collection.
   */
  virtual void GoToNextItem();

  /**
   * Test whether the iterator is finished with the traversal.
   * Returns 1 for yes, and 0 for no.
   * It is safe to call any of the GetCurrent...() methods only when
   * IsDoneWithTraversal() returns 0.
   */
  virtual bool IsDoneWithTraversal();

  /**
   * Returns the current session.
   */
  vtkSession* GetCurrentSession();

  /**
   * Returns the current session id.
   */
  vtkIdType GetCurrentSessionId();

protected:
  vtkSessionIterator();
  ~vtkSessionIterator();

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkSessionIterator(const vtkSessionIterator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSessionIterator&) VTK_DELETE_FUNCTION;
};

#endif
