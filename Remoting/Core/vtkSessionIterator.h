// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSessionIterator
 *
 * vtkSessionIterator is used to iterate over sessions in the global
 * ProcessModule.
 */

#ifndef vtkSessionIterator_h
#define vtkSessionIterator_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkSession;

class VTKREMOTINGCORE_EXPORT vtkSessionIterator : public vtkObject
{
public:
  static vtkSessionIterator* New();
  vtkTypeMacro(vtkSessionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  ~vtkSessionIterator() override;

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkSessionIterator(const vtkSessionIterator&) = delete;
  void operator=(const vtkSessionIterator&) = delete;
};

#endif
