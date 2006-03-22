/*=========================================================================

  Module:    vtkArrayMapIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArrayMapIterator - an array map iterator

#ifndef __vtkArrayMapIterator_h
#define __vtkArrayMapIterator_h

#include "vtkAbstractIterator.h"

template <class KeyType,class DataType>
class vtkArrayMapIterator : public vtkAbstractIterator<KeyType,DataType>
{
  friend class vtkArrayMap<KeyType,DataType>;
  virtual const char* GetClassNameInternal() const { return "vtkArrayMapIterator"; }
public:

  // Description:
  // Retrieve the index of the element.
  // This method returns VTK_OK if key was retrieved correctly.
  int GetKey(KeyType&);

  // Description:
  // Retrieve the data from the iterator. 
  // This method returns VTK_OK if key was retrieved correctly.
  int GetData(DataType&);

  // Description:
  // Initialize the traversal of the container. 
  // Set the iterator to the "beginning" of the container.
  void InitTraversal();

  // Description:
  // Check if the iterator is at the end of the container. Returns 1 for yes
  // and 0 for no.
  int IsDoneWithTraversal();

  // Description:
  // Increment the iterator to the next location.
  void GoToNextItem();

  // Description:
  // Decrement the iterator to the next location.
  void GoToPreviousItem();

  // Description:
  // Go to the "first" item of the map.
  void GoToFirstItem();

  // Description:
  // Go to the "last" item of the map.
  void GoToLastItem();

protected:
  static vtkArrayMapIterator<KeyType,DataType> *New(); 

  vtkArrayMapIterator() {
    this->Index = 0; 
  }
  virtual ~vtkArrayMapIterator() {}

  vtkIdType Index;

private:
  vtkArrayMapIterator(const vtkArrayMapIterator<KeyType,DataType>&); // Not implemented
  void operator=(const vtkArrayMapIterator<KeyType,DataType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkArrayMapIterator.txx"
#endif 

#endif



