/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCountedLinkedList.txx
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
// Include blockers needed since vtkReferenceCountedLinkedList.h 
// includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkReferenceCountedLinkedList_txx
#define __vtkReferenceCountedLinkedList_txx


#include "vtkReferenceCountedLinkedList.h"
#include "vtkLinkedList.txx"

//#define DEBUG_REGISTER() cout << "Register: " << __LINE__ << endl
//#define DEBUG_UNREGISTER() cout << "UnRegister: " << __LINE__ << endl
#define DEBUG_REGISTER()
#define DEBUG_UNREGISTER()

template <class DType>
class vtkReferenceCountedLinkedListNode : public vtkLinkedListNode<DType>
{
public:
  void DeleteAllReferenceCount()
    {
      if ( this->Data )
	{
	this->Data->UnRegister(0); DEBUG_UNREGISTER();
	}
      if ( this->Next )
	{
	static_cast<vtkReferenceCountedLinkedListNode<DType>*>(this->Next)
	  ->DeleteAllReferenceCount();
	delete this->Next;
	this->Next = 0;
	}
    }
};

// Description:
// Append an Item to the end of the vector
template <class DType>
int vtkReferenceCountedLinkedList<DType>::AppendItem(DType a) 
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
// Insert an Item to the front of the vector
template <class DType>
int vtkReferenceCountedLinkedList<DType>::PrependItem(DType a)
{
  int rc = a->GetReferenceCount();
  int res = this->Superclass::PrependItem(a);
  if ( res == VTK_OK && a->GetReferenceCount() == rc )
    {
    a->Register(0); DEBUG_REGISTER();
    }
  return res;
}
  
// Description:
// Insert an Item to the specific location in the vector.
template <class DType>
int vtkReferenceCountedLinkedList<DType>::InsertItem(vtkIdType loc, DType a)
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
// It also checks if the item can be set.
// It returns VTK_OK if successfull.
template <class DType>
int vtkReferenceCountedLinkedList<DType>::SetItem(vtkIdType loc, DType a)
{
  vtkLinkedListNode<DType> *curr = this->FindNode(loc);
  if ( !curr )
    {
    return VTK_ERROR;
    }

  if ( a != curr->Data )
    {
    curr->Data->UnRegister(0); DEBUG_UNREGISTER();
    curr->Data = a;
    a->Register(0); DEBUG_REGISTER();
    }

  return VTK_OK;
}

// Description:
// Sets the Item at the specific location in the list to a new value.
// It returns VTK_OK if successfull.
template <class DType>
void vtkReferenceCountedLinkedList<DType>::SetItemNoCheck(vtkIdType loc, 
							  DType a)
{
  vtkLinkedListNode<DType> *curr = this->FindNode(loc);
  if ( a != curr->Data )
    {
    curr->Data->UnRegister(0); DEBUG_UNREGISTER();
    curr->Data = a;
    a->Register(0); DEBUG_REGISTER();
    }
}

// Description:
// Remove an Item from the vector
template <class DType>
int vtkReferenceCountedLinkedList<DType>::RemoveItem(vtkIdType id) 
{
  vtkLinkedListNode<DType> *curr = 0;
  if ( !this->Head )
    {
    return VTK_ERROR;
    }
  if ( id == 0 )
    {
    // Fast delete from front
    curr = this->Head;
    this->Head = this->Head->Next;
    if ( !this->Head )
      {
      this->Tail = 0;
      }
    this->NumberOfItems--;
    curr->Data->UnRegister(0); DEBUG_UNREGISTER();
    delete curr;
    return VTK_OK;
    }
  // Find previous node
  curr = this->FindNode(id-1);
  if ( ! curr || ! curr->Next )
    {
    return VTK_ERROR;
    }
  // Set it to point to id+1 node
  vtkLinkedListNode<DType> *tmp = curr->Next;
  curr->Next = curr->Next->Next;
  if ( tmp == this->Tail )
    {
    // Fix tail
    this->Tail = curr;
    }
  // And delete id node
  tmp->Data->UnRegister(0); DEBUG_UNREGISTER();
  delete tmp;
  this->NumberOfItems --;
  return VTK_OK;
}
  
// Description:
// Removes all items from the container.
template <class DType>
void vtkReferenceCountedLinkedList<DType>::RemoveAllItems()
{
  if ( this->Head )
    {
    static_cast<vtkReferenceCountedLinkedListNode<DType>*>(this->Head)
      ->DeleteAllReferenceCount();
    delete this->Head;
    this->Head = 0;
    }
}

template <class DType>
vtkReferenceCountedLinkedList<DType>::~vtkReferenceCountedLinkedList()
{
  if ( this->Head )
    {
    static_cast<vtkReferenceCountedLinkedListNode<DType>*>(this->Head)
      ->DeleteAllReferenceCount();
    delete this->Head;
    this->Head = 0;
    }

  //if ( vtkReferenceCountedLinkedListNode<DType>::Count > 0 )
  //  {
  //  cout << "Number of nodes left: " 
  //	 << vtkReferenceCountedLinkedListNode<DType>::Count << endl;
  //  }
}

template <class DType>
void vtkReferenceCountedLinkedList<DType>::DebugList()
{
  vtkLinkedListNode<DType> *curr;
  int cc = 0; 
  cout << "List: " << this->GetClassName() << endl;
  for ( curr = this->Head; curr; curr = curr->Next )
    {
    cout << "Node [" << cc << "]: " << curr << " Next: " << curr->Next 
	 << " Data: " << curr->Data->GetClassName() << endl;
    cc ++;
    }
}

#endif
