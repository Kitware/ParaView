/*=========================================================================

  Module:    vtkQueue.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQueue - a link-list based templated queue

#ifndef __vtkQueue_h
#define __vtkQueue_h

#include "vtkVector.h"

template <class DType> class vtkQueueIterator;

template <class DType>
class vtkQueue : public vtkVector<DType>
{
  friend class vtkQueueIterator<DType>;
  virtual const char* GetClassNameInternal() const { return "vtkQueue"; }
public:
  typedef vtkVector<DType> Superclass;

  static vtkQueue<DType> *New();

  // Description:
  // Create the iterator.
  vtkQueueIterator<DType>* NewQueueIterator();

  // Description:
  // Enqueue an item.
  int EnqueueItem(DType a);

  // Description:
  // Dequeue the item.
  int DequeueItem();

  // Description:
  // Get the item to be deueued. This has to be done before calling of
  // DequeueItem
  int GetDequeueItem(DType &a);

  // Description:
  // Display the content of the list.
  virtual void DebugList();

  // Description:
  // Make queue empty.
  void MakeEmpty();

protected:
  vtkQueue();
  virtual ~vtkQueue();

  vtkIdType Start;
  vtkIdType End;

private:
  vtkQueue(const vtkQueue<DType>&); // Not implemented
  void operator=(const vtkQueue<DType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkQueue.txx"
#endif 

#endif



