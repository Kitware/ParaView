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
int vtkVector<DType>::AppendItem(DType a) 
{
  if ((this->NumberOfItems + 1) > this->Size)
    {
    if ( !this->Resize )
      {
      return VTK_ERROR;
      }

    if (!this->Size)
      {
      this->Size = 2;
      }
    DType *newArray = new DType [this->Size*2];
    unsigned long i;
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
  return VTK_OK;
}
  
// Description:
// Insert an Item to the front of the vector
template <class DType>
int vtkVector<DType>::PrependItem(DType a)
{
  return this->InsertItem(0, a);
}
  
// Description:
// Insert an Item to the specific location in the vector.
template <class DType>
int vtkVector<DType>::InsertItem(unsigned long loc, DType a)
{
  unsigned long i;
  if ((this->NumberOfItems + 1) > this->Size)
    {
    if ( !this->Resize )
      {
      return VTK_ERROR;
      }

    if (!this->Size)
      {
      this->Size = 2;
      }
    DType *newArray = new DType [this->Size*2];
    for (i = 0; i < this->NumberOfItems; ++i)
      {
      newArray[i+1] = this->Array[i];
      }
    this->Size = this->Size*2;
    if (this->Array)
      {
      delete [] this->Array;
      }
    this->Array = newArray;
    }
  else
    {
    for ( i = this->NumberOfItems; i > loc; i-- )
      {
      this->Array[i] = this->Array[i-1];
      }
    }
  this->Array[loc] = a;
  this->NumberOfItems++;
  return VTK_OK;
}

// Description:
// Remove an Item from the vector
template <class DType>
int vtkVector<DType>::RemoveItem(unsigned long id) 
{
  if (id >= this->NumberOfItems)
    {
    return VTK_ERROR;
    }
  unsigned long i;
  this->NumberOfItems--;
  
  if ( this->NumberOfItems < (this->Size / 3) && this->Size > 10 &&
       !this->Resize )
    {
    // We should resize the array not to waste space
    DType *newArray = new DType [ this->Size / 2 ];

    // Copy 0 - (id-1) elements
    for ( i = 0; i < id; ++i )
      {
      newArray[i] = this->Array[i];
      }
    // Copy (id+1) - end elements
    for ( i = id; i < this->NumberOfItems; i++ )
      {
      newArray[i] = this->Array[i+1];
      }
    delete [] this->Array;
    this->Array = newArray;
    }
  else
    {
    for (i = id; i < this->NumberOfItems; ++i)
      {
      this->Array[i] = this->Array[i+1];
      }
    }
  return VTK_OK;
}
  
// Description:
// Return an item that was previously added to this vector. 
template <class DType>
int vtkVector<DType>::GetItem(unsigned long id, DType& ret) 
{
  ret = 0;
  if (id < this->NumberOfItems)
    {
    ret = this->Array[id];
    return VTK_OK;
    }
  return VTK_ERROR;
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
      return VTK_OK;
      }
    }
  return VTK_ERROR;
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
      return VTK_OK;
      }
    }
  return VTK_ERROR;
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

// Description:
// Set the capacity of the vector.
// It returns VTK_OK if successfull.
// If capacity is set, the vector will not resize down.
template <class DType>
int vtkVector<DType>::SetSize(unsigned long size)
{
  if ( size < this->GetNumberOfItems() )
    {
    return VTK_ERROR;
    }
  this->ResizeOff();
  DType *newArray = new DType[ size ];
  if ( this->Array )
    {
    unsigned long cc;
    for ( cc = 0; cc < this->GetNumberOfItems(); cc ++ )
      {
      newArray[cc] = this->Array[cc];
      }
    delete [] this->Array;
    }
  this->Array = newArray;
  this->Size = size;
  return VTK_OK;
}
 
#endif
