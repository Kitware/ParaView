/*=========================================================================

  Module:    vtkHashMapIterator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHashMapIterator - an array map iterator

#ifndef __vtkHashMapIterator_h
#define __vtkHashMapIterator_h

#include "vtkAbstractIterator.h"

template <class DataType> class vtkVectorIterator;
template <class KeyType, class DataType> class vtkHashMap;
template <class KeyType, class DataType> class vtkAbstractMapItem;

template <class KeyType,class DataType>
class vtkHashMapIterator : public vtkAbstractIterator<KeyType,DataType>
{
  friend class vtkHashMap<KeyType,DataType>;
  virtual const char* GetClassNameInternal() const { return "vtkHashMapIterator"; }

public:

  // Description:
  // Retrieve the key of the current element.
  // This method returns VTK_OK if key was retrieved correctly.
  int GetKey(KeyType&);
  
  // Description:
  // Retrieve the data of the current element.
  // This method returns VTK_OK if key was retrieved correctly.
  int GetData(DataType&);
  
  // Description:
  // Retrieve the key and data of the current element.
  // This method returns VTK_OK if key and data were retrieved correctly.
  int GetKeyAndData(KeyType&, DataType&);
  
  // Description:
  // Set the data at the iterator's position.
  // This method returns VTK_OK if data were set correctly.
  int SetData(const DataType&);
  
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
  // Go to the "first" item of the map.
  void GoToFirstItem();
  
  // Description:
  // Go to the "last" item of the map.
  void GoToLastItem();
  
protected:
  static vtkHashMapIterator<KeyType,DataType> *New(); 
  
  vtkHashMapIterator();
  virtual ~vtkHashMapIterator();
  
  void ScanForward();
  void ScanBackward();
  
  typedef vtkAbstractMapItem<KeyType,DataType> ItemType;
  typedef vtkVectorIterator<ItemType> BucketIterator;
  
  vtkIdType Bucket;
  BucketIterator* Iterator;
  
private:
  vtkHashMapIterator(const vtkHashMapIterator<KeyType,DataType>&); // Not implemented
  void operator=(const vtkHashMapIterator<KeyType,DataType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkHashMapIterator.txx"
#endif 

#endif



