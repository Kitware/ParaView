/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.txx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkVector.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkVector_txx
#define __vtkVector_txx

#include "vtkVector.h"

// Description:
// Append an Item to the end of the vector
template <class DType>
unsigned long vtkVector<DType>::AppendItem(DType a) 
{
  if ((this->NumberOfItems + 1) >= this->Size)
    {
    if (!this->Size)
      {
      this->Size = 2;
      }
    DType *newArray = new DType [this->Size*2];
    unsigned int i;
    for (i = 0; i < this->NumberOfItems; ++i)
      {
      newArray[i] = this->Array[i];
      }
    this->Size = this->Size*2;
    if (this->Array)
      {
      delete [] this->Array;
      }
    this->Array = newArray;
    }
  this->Array[this->NumberOfItems] = a;
  this->NumberOfItems++;
  return (this->NumberOfItems - 1);
}
  
// Description:
// Remove an Item from the vector
template <class DType>
unsigned long vtkVector<DType>::RemoveItem(unsigned long id) 
{
  if (id >= this->NumberOfItems)
    {
    return 0;
    }
  unsigned int i;
  this->NumberOfItems--;
  for (i = id; i < this->NumberOfItems; ++i)
    {
    this->Array[i] = this->Array[i+1];
    }
  return 1;
}
  
// Description:
// Return an item that was previously added to this vector. 
template <class DType>
int vtkVector<DType>::GetItem(unsigned long id, DType& ret) 
{
  if (id < this->NumberOfItems)
    {
    ret = this->Array[id];
    return 1;
    }
  return 0;
}
      
// Description:
// Find an item in the vector. Return one if it was found, zero if it was
// not found. The location of the item is returned in res.
template <class DType>
int vtkVector<DType>::Find(DType a, unsigned long &res) 
{
  unsigned long i;
  for (i = 0; i < this->NumberOfItems; ++i)
    {
    if (this->Array[i] == a)
      {
      res = i;
      return 1;
      }
    }
  return 0;
}

// Description:
// Find an item in the vector using a comparison routine. 
// Return one if it was found, zero if it was
// not found. The location of the item is returned in res.
template <class DType>
int vtkVector<DType>::Find(DType a, 
			   vtkAbstractList<DType>::CompareFunction compare, 
			   unsigned long &res) 
{
  unsigned long i;
  for (i = 0; i < this->NumberOfItems; ++i)
    {
    if ( compare(this->Array[i], a) )
      {
      res = i;
      return 1;
      }
    }
  return 0;
}
  

// Description:
// Removes all items from the container.
template <class DType>
void vtkVector<DType>::RemoveAllItems()
{
  if (this->Array)
    {
    delete [] this->Array;
    }
  this->Array = 0;
  this->NumberOfItems = 0;
  this->Size = 0;
}
  
#endif
