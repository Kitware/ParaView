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
// .NAME vtkSessionIterator
// .SECTION Description
// vtkSessionIterator is used to iterate over sessions in the global
// ProcessModule.

#ifndef __vtkSessionIterator_h
#define __vtkSessionIterator_h

#include "vtkObject.h"

class vtkSession;

class VTK_EXPORT vtkSessionIterator : public vtkObject
{
public:
  static vtkSessionIterator* New();
  vtkTypeMacro(vtkSessionIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin iterating over the composite dataset structure.
  virtual void InitTraversal();

  // Description:
  // Move the iterator to the next item in the collection.
  virtual void GoToNextItem();

  // Description:
  // Test whether the iterator is finished with the traversal.
  // Returns 1 for yes, and 0 for no.
  // It is safe to call any of the GetCurrent...() methods only when
  // IsDoneWithTraversal() returns 0.
  virtual bool IsDoneWithTraversal();

  // Description:
  // Returns the current session.
  vtkSession* GetCurrentSession();

  // Description:
  // Returns the current session id.
  vtkIdType GetCurrentSessionId();

//BTX
protected:
  vtkSessionIterator();
  ~vtkSessionIterator();

  class vtkInternals;
  vtkInternals* Internals;
private:
  vtkSessionIterator(const vtkSessionIterator&); // Not implemented
  void operator=(const vtkSessionIterator&); // Not implemented
//ETX
};

#endif
