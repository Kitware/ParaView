/*=========================================================================

  Module:    vtkQueueIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQueueIterator - a templated vector list iterator

#ifndef __vtkQueueIterator_h
#define __vtkQueueIterator_h

#include "vtkAbstractIterator.h"

template <class DType> class vtkLinkedList;
template <class DType> class vtkLinkedListNode;

template <class DType>
class vtkQueueIterator : public vtkAbstractIterator<vtkIdType,DType>
{
  friend class vtkQueue<DType>;
  virtual const char* GetClassNameInternal() const { return "vtkQueueIterator"; }

public:

  // Description:
  // Retrieve the index of the element.
  // This method returns VTK_OK if key was retrieved correctly.
  int GetKey(vtkIdType&);

  // Description:
  // Retrieve the data from the iterator. 
  // This method returns VTK_OK if data were retrieved correctly.
  int GetData(DType&);
  
  // Description:
  // Set the data at the iterator's position.
  // This method returns VTK_OK if data were set correctly.
  int SetData(const DType&);
  
  // Description:
  // Initialize the traversal of the container. 
  // Set the iterator to the "beginning" of the container.
  void InitTraversal();
  
  // Description:
  // Check if the iterator is at the end of the container. Returns 1
  // for yes and 0 for no.
  int IsDoneWithTraversal();
  
  // Description:
  // Increment the iterator to the next location.
  void GoToNextItem();
  
  // Description:
  // Decrement the iterator to the next location.
  void GoToPreviousItem();
  
  // Description:
  // Go to the first item of the list.
  void GoToFirstItem();
  
  // Description:
  // Go to the last item of the list.
  void GoToLastItem();
  
protected:
  static vtkQueueIterator<DType> *New(); 

  vtkQueueIterator() {
    this->Index = -1;
    this->Number = 0;
  }
  virtual ~vtkQueueIterator() {}

  vtkIdType Index;
  vtkIdType Number;

private:
  vtkQueueIterator(const vtkQueueIterator&); // Not implemented
  void operator=(const vtkQueueIterator&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkQueueIterator.txx"
#endif 

#endif



