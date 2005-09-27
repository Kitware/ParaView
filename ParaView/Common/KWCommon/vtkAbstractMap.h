/*=========================================================================

  Module:    vtkAbstractMap.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractMap - a dynamic map data structure
// .SECTION Description
// vtkAbstractMap is a an abstract templated superclass of all
// containers that implement map data structure.
//
// Map data structure is a one dimensional set of pairs. Each pair 
// contains a key and associated data. On the higher level, it 
// implements mapping from key values to data elements. It can be 
// implemented using array of pairs, hash table, or different trees.

// .SECTION See also
// vtkContainer vtkAbstractList

#include "vtkContainer.h"

#ifndef __vtkAbstractMap_h
#define __vtkAbstractMap_h

// This is an item of the map.
template<class KeyType, class DataType>
class vtkAbstractMapItem
{
public:
  KeyType Key;
  DataType Data;
};

template<class KeyType, class DataType>
class vtkAbstractIterator;

template<class KeyType, class DataType>
class vtkAbstractMap : public vtkContainer
{
  virtual const char* GetClassNameInternal() const {return "vtkAbstractMap";}
public:
  typedef vtkContainer Superclass; 

  // Description:
  // Sets the item at with specific key to data.
  // It overwrites the old item.
  // It returns VTK_OK if successfull.
  //virtual int SetItem(const KeyType& key, const DataType& data) = 0;
  
  // Description:
  // Remove an Item with the key from the map.
  // It returns VTK_OK if successfull.
  //virtual int RemoveItem(const KeyType& key) = 0;
  
  // Description:
  // Return the data asociated with the key.
  // It returns VTK_OK if successfull.
  //virtual int GetItem(const KeyType& key, DataType& data) = 0;
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  //virtual vtkIdType GetNumberOfItems() = 0;  

protected:
  vtkAbstractMap();

private:
  vtkAbstractMap(const vtkAbstractMap&); // Not implemented
  void operator=(const vtkAbstractMap&); // Not implemented
};

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
int vtkContainerCompareMethod(const vtkAbstractMapItem<KeyType, DataType>& d1,
                              const vtkAbstractMapItem<KeyType, DataType>& d2)
{
  // Use only the Key for comparison.
  return ::vtkContainerCompareMethod(d1.Key, d2.Key);
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
vtkAbstractMapItem<KeyType, DataType>
vtkContainerCreateMethod(const vtkAbstractMapItem<KeyType, DataType>& item)
{
  // Copy both components from the input.
  vtkAbstractMapItem<KeyType, DataType> result =
    {
      static_cast<KeyType>(vtkContainerCreateMethod(item.Key)),
      static_cast<DataType>(vtkContainerCreateMethod(item.Data))
    };
  return result;
}

//----------------------------------------------------------------------------
template<class KeyType, class DataType>
void vtkContainerDeleteMethod(vtkAbstractMapItem<KeyType, DataType>& item)
{
  // Delete both components.
  vtkContainerDeleteMethod(item.Key);
  vtkContainerDeleteMethod(item.Data);
}

//----------------------------------------------------------------------------

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkAbstractMap.txx"
#endif 

#endif



