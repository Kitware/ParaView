// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVSessionIterator
 *
 * vtkPVSessionIterator is used to iterate over sessions in the global
 * ProcessModule.
 */

#ifndef vtkPVSessionIterator_h
#define vtkPVSessionIterator_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" //needed for exports

class vtkPVSession;

class VTKREMOTINGCORE_EXPORT vtkPVSessionIterator : public vtkObject
{
public:
  static vtkPVSessionIterator* New();
  vtkTypeMacro(vtkPVSessionIterator, vtkObject);
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
  vtkPVSession* GetCurrentSession();

  /**
   * Returns the current session id.
   */
  vtkIdType GetCurrentSessionId();

protected:
  vtkPVSessionIterator();
  ~vtkPVSessionIterator() override;

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkPVSessionIterator(const vtkPVSessionIterator&) = delete;
  void operator=(const vtkPVSessionIterator&) = delete;
};

#endif
