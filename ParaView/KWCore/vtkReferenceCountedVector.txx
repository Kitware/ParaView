/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCountedVector.txx
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
// Include blockers needed since vtkReferenceCountedVector.h includes 
// this file when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkReferenceCountedVector_txx
#define __vtkReferenceCountedVector_txx

#include "vtkReferenceCountedVector.h"
#include "vtkVector.txx"

//#define DEBUG_REGISTER() cout << "Register: " << __LINE__ << endl
//#define DEBUG_UNREGISTER() cout << "UnRegister: " << __LINE__ << endl
#define DEBUG_REGISTER()
#define DEBUG_UNREGISTER()

// Description:
// Append an Item to the end of the vector
template <class DType>
int vtkReferenceCountedVector<DType>::AppendItem(DType a) 
{
  int rc = a->GetReferenceCount();
  int res = this->Superclass::AppendItem(a);
  if ( res == VTK_OK && a->GetReferenceCount() == rc )
    {
    a->Register(0); DEBUG_REGISTER();
    }
  return res;
}
  
// Description:
// Insert an Item to the specific location in the vector.
template <class DType>
int vtkReferenceCountedVector<DType>::InsertItem(vtkIdType loc, DType a)
{
  int rc = a->GetReferenceCount();
  int res = this->Superclass::InsertItem(loc, a);
  if ( res == VTK_OK && a->GetReferenceCount() == rc )
    {
    a->Register(0); DEBUG_REGISTER();
    }
  return res;
}

// Description:
// Sets the Item at the specific location in the list to a new value.
// It returns VTK_OK if successfull.
template <class DType>
void vtkReferenceCountedVector<DType>::SetItemNoCheck(vtkIdType loc, DType a)
{  
  if ( a != this->Array[loc] )
    {
    if ( this->Array[loc] )
      {
      this->Array[loc]->UnRegister(0); DEBUG_UNREGISTER();
      }
    this->Array[loc] = a;
    a->Register(0); DEBUG_REGISTER();
    }
}

// Description:
// Remove an Item from the vector
template <class DType>
int vtkReferenceCountedVector<DType>::RemoveItem(vtkIdType id) 
{
  if ( id >= this->NumberOfItems )
    {
    return VTK_ERROR;
    }
  DType a = this->Array[id];
  int res = this->Superclass::RemoveItem(id);
  if ( res == VTK_OK && a )
    {
    a->UnRegister(0); DEBUG_UNREGISTER();
    }
  return res;
}
  
// Description:
// Removes all items from the container.
template <class DType>
void vtkReferenceCountedVector<DType>::RemoveAllItems()
{
  int cc;
  for ( cc = 0; cc < this->NumberOfItems; cc ++ )
    {
    if ( this->Array[cc] )
      {
      if ( this->Array[cc] )
	{
	this->Array[cc]->UnRegister(0); DEBUG_REGISTER();
	}
      }
    }
  this->Superclass::RemoveAllItems();
}

// Description:
// Destructor.
template <class DType>
vtkReferenceCountedVector<DType>::~vtkReferenceCountedVector()
{
  int cc;
  for ( cc = 0; cc < this->NumberOfItems; cc ++ )
    {
    if ( this->Array[cc] )
      {
      if ( this->Array[cc] )
	{
	this->Array[cc]->UnRegister(0); DEBUG_UNREGISTER();
	}
      }
    }
}

#endif
