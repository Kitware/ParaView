/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// Include blockers needed since vtkLinkedList.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkLinkedList_txx
#define __vtkLinkedList_txx

#include "vtkLinkedList.h"

#include "vtkAbstractList.txx"
#include "vtkDebugLeaks.h"
#include "vtkLinkedListIterator.txx"

template <class DType>
vtkLinkedList<DType> *vtkLinkedList<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkLinkedList");
#endif
  return new vtkLinkedList<DType>(); 
}

template <class DType>
class vtkLinkedListNode 
{
public:
  vtkLinkedListNode() { this->Next = 0; }
  ~vtkLinkedListNode() {}
  void DeleteAll()
    {
      ::vtkContainerDeleteMethod(this->Data);
      if ( this->Next )
        {
        this->Next->DeleteAll();
        delete this->Next;
        this->Next = 0;
        }
    }

  DType Data;
  vtkLinkedListNode<DType> *Next;
};

// Description:
// Append an Item to the end of the vector
template <class DType>
int vtkLinkedList<DType>::AppendItem(DType a) 
{
  if ( !this->Tail )
    {
    return this->PrependItem(a);
    }
  vtkLinkedListNode<DType> *node = new vtkLinkedListNode<DType>;
  if ( !node )
    {
    return VTK_ERROR;
    }  
  node->Data = static_cast<DType>( ::vtkContainerCreateMethod(a) );
  this->Tail->Next = node;
  this->Tail = node;
  this->NumberOfItems ++;
  return VTK_OK;
}
  
// Description:
// Insert an Item to the front of the vector
template <class DType>
int vtkLinkedList<DType>::PrependItem(DType a)
{
  vtkLinkedListNode<DType> *node = new vtkLinkedListNode<DType>;
  if ( !node )
    {
    return VTK_ERROR;
    }
  node->Data = static_cast<DType>( ::vtkContainerCreateMethod(a) );
  node->Next = this->Head;
  this->Head = node;
  if ( !this->Tail )
    {
    this->Tail = this->Head;
    }
  this->NumberOfItems ++;
  return VTK_OK;
}
  
// Description:
// Insert an Item to the specific location in the vector.
template <class DType>
int vtkLinkedList<DType>::InsertItem(vtkIdType loc, DType a)
{
  if ( loc > this->NumberOfItems )
    {
    return VTK_ERROR;
    }
  if ( loc == 0 )
    {
    return this->PrependItem(a);
    }
  if ( loc == this->NumberOfItems )
    {
    return this->AppendItem(a);
    }
  vtkLinkedListNode<DType> *curr = this->FindNode(loc-1);
  if ( curr )
    {
    vtkLinkedListNode<DType> *node = new vtkLinkedListNode<DType>;
    if ( !node )
      {
      return VTK_ERROR;      
      }
    node->Next = curr->Next;
    node->Data = static_cast<DType>( ::vtkContainerCreateMethod(a) );
    curr->Next = node;
    this->NumberOfItems++;
    return VTK_OK;
    }
  return VTK_ERROR;
}

// Description:
// Sets the Item at the specific location in the list to a new value.
// It also checks if the item can be set.
// It returns VTK_OK if successfull.
template <class DType>
int vtkLinkedList<DType>::SetItem(vtkIdType loc, DType a)
{
  vtkLinkedListNode<DType> *curr = this->FindNode(loc);
  if ( !curr )
    {
    return VTK_ERROR;
    }
  ::vtkContainerDeleteMethod(curr->Data);
  curr->Data = static_cast<DType>( ::vtkContainerCreateMethod(a) );
  return VTK_OK;
}

// Description:
// Sets the Item at the specific location in the list to a new value.
// It returns VTK_OK if successfull.
template <class DType>
void vtkLinkedList<DType>::SetItemNoCheck(vtkIdType loc, DType a)
{
  vtkLinkedListNode<DType> *curr = this->FindNode(loc);
  ::vtkContainerDeleteMethod(curr->Data);
  curr->Data = static_cast<DType>( ::vtkContainerCreateMethod(a) );
}

// Description:
// Remove an Item from the vector
template <class DType>
int vtkLinkedList<DType>::RemoveItem(vtkIdType id) 
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
    ::vtkContainerDeleteMethod(curr->Data);
    delete curr;
    if ( !this->Head )
      {
      this->Tail = 0;
      }
    this->NumberOfItems--;
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
  ::vtkContainerDeleteMethod(tmp->Data);
  delete tmp;
  this->NumberOfItems --;
  return VTK_OK;
}
  
// Description:
// Return an item that was previously added to this vector. 
template <class DType>
int vtkLinkedList<DType>::GetItem(vtkIdType id, DType& ret) 
{
  vtkLinkedListNode<DType> *curr = this->FindNode(id);
  if ( !curr )
    {
    return VTK_ERROR;
    } 
  ret = curr->Data;
  return VTK_OK;
}
      
// Description:
// Find an item in the vector. Return one if it was found, zero if it was
// not found. The location of the item is returned in res.
template <class DType>
int vtkLinkedList<DType>::FindItem(DType a, vtkIdType &res) 
{
  vtkIdType cc = 0;
  vtkLinkedListNode<DType> *curr;
  for ( curr = this->Head; curr; curr = curr->Next, cc++ )
    {
    if ( ::vtkContainerCompareMethod(curr->Data, a) == 0 )
      {
      res = cc;
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
int vtkLinkedList<DType>::FindItem(
  DType a, 
  vtkAbstractListCompareFunction(DType, compare),
  vtkIdType &res) 
{
  vtkIdType cc = 0;
  vtkLinkedListNode<DType> *curr;
  for ( curr = this->Head; curr; curr = curr->Next, cc++  )
    {
    if ( compare(curr->Data, a) == 0 )
      {
      res = cc;
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}
  

// Description:
// Removes all items from the container.
template <class DType>
void vtkLinkedList<DType>::RemoveAllItems()
{
  if ( this->Head )
    {
    this->Head->DeleteAll();
    delete this->Head;
    this->Head = 0;
    this->Tail = 0;
    this->NumberOfItems = 0;
    }
}

template <class DType>
vtkLinkedListNode<DType>* vtkLinkedList<DType>::FindNode(vtkIdType i)
{
  vtkIdType cc = 0;
  vtkLinkedListNode<DType> *curr = 0;
  for ( curr=this->Head; curr; curr = curr->Next )
    {
    if ( cc == i )
      {
      return curr;
      }
    cc++;
    }
  return 0;
}

template <class DType>
vtkLinkedList<DType>::~vtkLinkedList()
{
  if ( this->Head )
    {
    this->Head->DeleteAll();
    delete this->Head;
    }
}

template <class DType>
void vtkLinkedList<DType>::DebugList()
{
  vtkLinkedListNode<DType> *curr;
  int cc = 0; 
  cout << "List: " << this->GetClassName() << endl;
  for ( curr = this->Head; curr; curr = curr->Next )
    {
    cout << "Node [" << cc << "]: " << curr << " Next: " << curr->Next 
         << " Data: " << curr->Data << endl;
    cc ++;
    }
}

template <class DType>
vtkLinkedListIterator<DType> *vtkLinkedList<DType>::NewIterator()
{
  vtkLinkedListIterator<DType> *it = vtkLinkedListIterator<DType>::New();
  it->SetContainer(this);
  it->InitTraversal();
  return it;
}

#if defined ( _MSC_VER )
template <class DType>
vtkLinkedList<DType>::vtkLinkedList(const vtkLinkedList<DType>&){}
template <class DType>
void vtkLinkedList<DType>::operator=(const vtkLinkedList<DType>&){}
#endif

#endif



