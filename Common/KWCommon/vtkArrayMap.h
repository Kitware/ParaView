/*=========================================================================

  Module:    vtkArrayMap.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArrayMap - a dynamic map data structure
// .SECTION Description
// vtkArrayMap is a an array implementation of the map data structure
//
// Map data structure is a one dimensional sequence of pairs
// of key and data. On the higher level, it implements mapping
// from key values to data elements. It can be implemented using
// array of pairs, hash table, or different trees.

// .SECTION See also
// vtkAbstractMap

#include "vtkAbstractMap.h"

#ifndef __vtkArrayMap_h
#define __vtkArrayMap_h

template<class DataType> class vtkVector;
template<class KeyType, class DataType> class vtkArrayMapIterator;

template<class KeyType, class DataType>
class vtkArrayMap : public vtkAbstractMap<KeyType,DataType>
{
  friend class vtkArrayMapIterator<KeyType,DataType>;
  virtual const char* GetClassNameInternal() const {return "vtkArrayMap";}
public:
  typedef vtkAbstractMap<KeyType,DataType> Superclass;
  typedef vtkArrayMapIterator<KeyType,DataType> IteratorType;

  // Cannot use this macro because of the comma in the type name.
  // The CPP splits that in two and we ae in trouble.
  //vtkContainerTypeMacro((vtkArrayMap<KeyType,DataType>), vtkContainer);

  static vtkArrayMap<KeyType,DataType> *New(); 

  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkArrayMapIterator<KeyType,DataType> *NewIterator();

  // Description:
  // Sets the item at with specific key to data.
  // It overwrites the old item.
  // It returns VTK_OK if successfull.
  int SetItem(const KeyType& key, const DataType& data);
  
  // Description:
  // Remove an Item with the key from the map.
  // It returns VTK_OK if successfull.
  int RemoveItem(const KeyType& key);
  
  // Description:
  // Remove all items from the map.
  void RemoveAllItems();
  
  // Description:
  // Return the data asociated with the key.
  // It returns VTK_OK if successfull.
  int GetItem(const KeyType& key, DataType& data);
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  vtkIdType GetNumberOfItems() const;

  // Description:
  // Find an item in the vector. Return one if it was found, zero if it was
  // not found.
  int IsItemPresent(const KeyType& key);

  void DebugList();

protected:
  vtkArrayMap() { this->Array = 0; }
  virtual ~vtkArrayMap();

  // Description:
  // Find vtkAbstractMapItem that with specific key
  virtual vtkAbstractMapItem<KeyType,DataType> 
    *FindDataItem(const KeyType key);

  vtkVector< vtkAbstractMapItem<KeyType,DataType>* > *Array;

private:
  vtkArrayMap(const vtkArrayMap<KeyType,DataType>&); // Not implemented
  void operator=(const vtkArrayMap<KeyType,DataType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkArrayMap.txx"
#endif 

#endif



