/*=========================================================================

  Module:    vtkVector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVector - a dynamic vector

#ifndef __vtkVector_h
#define __vtkVector_h

#include "vtkAbstractList.h"

extern "C" 
{ 
  typedef int (*vtkVectorSortFunctionType)(const void *, const void *);
}

template <class DType> class vtkVectorIterator;

template <class DType>
class vtkVector : public vtkAbstractList<DType>
{
  friend class vtkVectorIterator<DType>;
  virtual const char* GetClassNameInternal() const { return "vtkVector"; }

public:
  typedef vtkAbstractList<DType> Superclass;
  typedef vtkVectorIterator<DType> IteratorType;
  
  static vtkVector<DType> *New();

  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkVectorIterator<DType> *NewIterator();
  
  // Description:
  // Append an Item to the end of the vector.
  int AppendItem(DType a);
  
  // Description:
  // Make a copy of another conatiner.
  void CopyItems(vtkVector<DType> *in);

  // Description:
  // Insert an Item to the front of the linked list.
  int PrependItem(DType a);
  
  // Description:
  // Insert an Item to the specific location in the vector.  
  // Any items in the vector at a location greater than loc will be
  // shifted by one position to make room for the inserted item.
  // NOTE: this can not be used with SetSize because there will
  // be no room for the additional item.
  int InsertItem(vtkIdType loc, DType a);
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // It also checks if the item can be set.
  // It returns VTK_OK if successfull.
  int SetItem(vtkIdType loc, DType a);

  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  void SetItemNoCheck(vtkIdType loc, DType a);

   // Description:
  // Remove an Item from the vector
  int RemoveItem(vtkIdType id);
  
  // Description:
  // Return an item that was previously added to this vector. 
  int GetItem(vtkIdType id, DType& ret);
      
  // Description:
  // Return an item that was previously added to this vector. 
  void GetItemNoCheck(vtkIdType id, DType& ret);
      
  // Description:
  // Find an item in the vector. Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  int FindItem(DType a, vtkIdType &res);

  // Description:
  // Find an item in the vector using a comparison routine. 
  // Return VTK_OK if it was found, VTK_ERROR if it was
  // not found. The location of the item is returned in res.
  int FindItem(DType a, 
               vtkAbstractListCompareFunction(DType, compare), 
               vtkIdType &res);
  
  // Description:
  // Find an item in the vector. Return one if it was found, zero if it was
  // not found. 
  int IsItemPresent(DType a);

  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  vtkIdType GetNumberOfItems() const { return this->NumberOfItems; }
  
  // Description:
  // Returns the number of items the container can currently hold.
  vtkIdType GetSize() const { return this->Size; }

  // Description:
  // Removes all items from the container.
  void RemoveAllItems();

  // Description:
  // Set the capacity of the vector.
  // It returns VTK_OK if successfull.
  // If capacity is set, the vector will not resize.
  int SetSize(vtkIdType size);

  // Description:
  // Allow or disallow resizing. If resizing is disallowed, when
  // inserting too many elements, it will return VTK_ERROR.
  // Initially allowed.
  void SetResize(int r) { this->Resize = r; }
  void ResizeOn() { this->SetResize(1); }
  void ResizeOff() { this->SetResize(0); }
  int GetResize() const { return this->Resize; }

  // Description:
  // Display the content of the list.
  void DebugList();

  // Description:
  // Sort the content of the list.
  // Provide a comparison function (see def of vtkVectorSortFunctionType).
  void Sort(vtkVectorSortFunctionType);

protected:
  vtkVector() {
    this->Array = 0; this->NumberOfItems = 0; this->Size = 0; 
    this->Resize = 1; }
  virtual ~vtkVector();
  vtkIdType NumberOfItems;
  vtkIdType Size;
  int Resize;
  DType *Array;

private:
  vtkVector(const vtkVector<DType>&); // Not implemented
  void operator=(const vtkVector<DType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkVector.txx"
#endif 

#endif



