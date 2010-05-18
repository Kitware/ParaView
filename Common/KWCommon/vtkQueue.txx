/*=========================================================================

  Module:    vtkQueue.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkQueue.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.
#ifndef __vtkQueue_txx
#define __vtkQueue_txx

#include "vtkQueue.h"
#include "vtkQueueIterator.txx"
#include "vtkVector.txx"

#include "vtkDebugLeaks.h"

template <class DType>
vtkQueue<DType> *vtkQueue<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkQueue");
#endif
  return new vtkQueue<DType>(); 
}

template <class DType>
vtkQueueIterator<DType>* vtkQueue<DType>::NewQueueIterator()
{
  vtkQueueIterator<DType>* it = vtkQueueIterator<DType>::New();
  it->SetContainer(this);
  it->InitTraversal();
  return it;
}

template <class DType>
vtkQueue<DType>::vtkQueue()
{
  const vtkIdType initial_size = this->GetSize();
  this->Start = initial_size - 1;
  this->End = 0;
  //this->SetSize(initial_size);
  this->NumberOfItems = 0;
}

template <class DType>
vtkQueue<DType>::~vtkQueue() 
{
  this->MakeEmpty();
}

template <class DType>
int vtkQueue<DType>::EnqueueItem(DType a)
{
  if ( this->GetSize() == 0 ||
    (this->End == (this->Start + 1) % this->GetSize() &&
       this->NumberOfItems > 0 ) )
    {
    vtkIdType newsize = (this->Size+1) * 2;
    vtkIdType oldsize = this->Size;
    //cout << "Queue is full, resizing from: " << oldsize << " to " << newsize<< endl;
    DType* newarray = new DType[newsize];
    vtkIdType num = this->NumberOfItems;
    vtkIdType nidx = 0;
    if ( oldsize > 0 )
      {
      vtkIdType cc;
      for ( cc = this->End; nidx < num; cc = (cc + 1) % oldsize)
        {
        //cout << "Copy " <<  cc << " to " << nidx << endl;
        newarray[nidx] = this->Array[cc];
        nidx ++;
        }
      }
    this->End = 0;
    this->Start = nidx-1 % newsize;
    delete [] this->Array;
    this->Array = newarray;
    this->Size = newsize;
    }

  this->Start = (this->Start + 1) % this->GetSize();
  this->Array[this->Start] = static_cast<DType>( ::vtkContainerCreateMethod(a) );
  this->NumberOfItems ++;
  return VTK_OK;
}

template <class DType>
int vtkQueue<DType>::DequeueItem()
{
  if ( this->End == (this->Start +1) % this->GetSize() &&
       this->NumberOfItems == 0 )
    {
    //cout << "Queue is empty" << endl;
    return VTK_ERROR;
    }
  ::vtkContainerDeleteMethod(this->Array[this->End]);
  this->End = (this->End + 1) % this->GetSize();
  this->NumberOfItems --;
  return VTK_OK;
}

template <class DType>
int vtkQueue<DType>::GetDequeueItem(DType &a)
{
  if ( this->NumberOfItems == 0 )
    {
    return VTK_ERROR;
    }
  this->GetItemNoCheck(this->End, a);
  return VTK_OK;
}

template <class DType>
void vtkQueue<DType>::MakeEmpty()
{
  if ( this->NumberOfItems == 0 || 
       this->End == (this->Start +1) % this->GetSize() )
    {
    return;
    }
  vtkIdType cc;
  for ( cc = this->End; this->NumberOfItems > 0 ; cc = (cc + 1) % this->GetSize() )
    {
    ::vtkContainerDeleteMethod(this->Array[cc]);
    this->NumberOfItems --;
    }
  this->NumberOfItems = 0;
  this->Start = this->GetSize()-1;
  this->End = 0;
}

template <class DType>
void vtkQueue<DType>::DebugList()
{
  vtkIdType cc;
  cout << "List: " << this << " type: " << this->GetClassName() << endl;
  cout << "Number of items: " << this->GetNumberOfItems() 
       << " S: " << this->Start << " E: " << this->End << endl;
  for ( cc = 0; cc < this->GetSize(); cc ++ )
    {
    vtkIdType idx = 0;
    if ( this->End == (this->Start+1) % this->GetSize() && this->NumberOfItems == 0)
      {
      idx = -1;
      }
    else if ( this->Start < this->End )
      {
      if ( cc > this->Start && cc < this->End )
        {
        idx = -1;
        }
      else
        {
        if ( cc <= this->Start )
          {
          idx = cc + (this->GetSize() - this->End);
          }
        if ( cc >= this->End )
          {
          idx = (cc - this->End);
          }
        }
      }
    else // Start >= this->End
      {
      if ( cc < this->End || cc > this->Start )
        {
        idx = -1;
        }
      else
        {
        idx = (cc - this->End);
        }
      }
    if ( idx >= 0 )
      {
      cout << "Item [" << idx << " | " <<  cc << "]: " << this->Array[cc];
      }
    else
      {
      cout << "Item [" << idx << " | " <<  cc << "]: none";
      }
    if ( cc == this->Start )
      {
      cout << " <- start";
      }
    if ( cc == this->End )
      {
      cout << " <- end";
      }
    cout << endl;
    }
}

#if defined ( _MSC_VER )
template <class DType>
vtkQueue<DType>::vtkQueue(const vtkQueue<DType>&){}
template <class DType>
void vtkQueue<DType>::operator=(const vtkQueue<DType>&){}
#endif

#endif



