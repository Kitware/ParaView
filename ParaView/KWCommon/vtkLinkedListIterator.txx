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
// Include blockers needed since vtkVector.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkLinkedListIterator_txx
#define __vtkLinkedListIterator_txx

#include "vtkLinkedListIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkLinkedList.h"

template <class DType>
vtkLinkedListIterator<DType> *vtkLinkedListIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkLinkedListIterator");
#endif
  return new vtkLinkedListIterator<DType>(); 
}

template<class DType>
void vtkLinkedListIterator<DType>::InitTraversal()
{
  this->GoToFirstItem();
}

// Description:
// Retrieve the index of the element.
// This method returns VTK_OK if key was retrieved correctly.
template<class DType>
int vtkLinkedListIterator<DType>::GetKey(vtkIdType& key)
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }

  vtkLinkedListNode<DType> *curr;
  int cc = 0;
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  for ( curr = llist->Head; curr; curr = curr->Next )
    {
    if ( curr == this->Pointer )
      {
      key = cc;
      return VTK_OK;
      }
    cc ++;
    }
  return VTK_ERROR;
}

// Description:
// Retrieve the data from the iterator. 
// This method returns VTK_OK if key was retrieved correctly.
template<class DType>
int vtkLinkedListIterator<DType>::GetData(DType& data)
{
  if ( !this->Pointer )
    {
    return VTK_ERROR;
    }
  data = this->Pointer->Data;
  return VTK_OK;
}

template<class DType>
int vtkLinkedListIterator<DType>::IsDoneWithTraversal()
{
  if ( !this->Pointer )
    {
    return 1;
    }
  return 0;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToNextItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToFirstItem();
    }
  else
    {
    this->Pointer = this->Pointer->Next;
    }
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToPreviousItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToLastItem();
    return;
    }
  
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  
  // Fast exit if at beginning of the list
  if ( this->Pointer == llist->Head )
    {
    this->Pointer = 0;
    return;
    }

  // Traverse the list to find the previous node
  vtkLinkedListNode<DType> *curr = 0;
  for ( curr = llist->Head; curr ; curr = curr->Next )
    {
    if ( curr->Next == this->Pointer )
      {
      break;
      }
    }
  
  this->Pointer = curr;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToFirstItem()
{
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Head;
}

template<class DType>
void vtkLinkedListIterator<DType>::GoToLastItem()
{
  vtkLinkedList<DType> *llist 
    = static_cast<vtkLinkedList<DType>*>(this->Container);
  this->Pointer = llist->Tail;
}

#if defined ( _MSC_VER )
template <class DType>
vtkLinkedListIterator<DType>::vtkLinkedListIterator(const vtkLinkedListIterator<DType>&){}
template <class DType>
void vtkLinkedListIterator<DType>::operator=(const vtkLinkedListIterator<DType>&){}
#endif

#endif



