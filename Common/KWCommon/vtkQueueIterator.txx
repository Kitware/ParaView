/*=========================================================================

  Module:    vtkQueueIterator.txx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkQueue.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkQueueIterator_txx
#define __vtkQueueIterator_txx

#include "vtkQueueIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkQueue.h"

//----------------------------------------------------------------------------
template <class DType>
vtkQueueIterator<DType> *vtkQueueIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkQueueIterator");
#endif
  return new vtkQueueIterator<DType>(); 
}

//----------------------------------------------------------------------------
template<class DType>
void vtkQueueIterator<DType>::InitTraversal()
{
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
template<class DType>
int vtkQueueIterator<DType>::GetKey(vtkIdType& key)
{
  if ( this->Index == -1 )
    {
    return VTK_ERROR;
    }
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  if ( this->Number == llist->NumberOfItems ) { return VTK_ERROR; }
  key = this->Number;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkQueueIterator<DType>::GetData(DType& data)
{
  if ( this->Index == -1 )
    {
    return VTK_ERROR;
    }
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  data = llist->Array[this->Index];
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkQueueIterator<DType>::SetData(const DType& data)
{
  if ( this->Index == -1 )
    {
    return VTK_ERROR;
    }
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  llist->Array[this->Index] = data;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkQueueIterator<DType>::IsDoneWithTraversal()
{
  if ( this->Index == -1 )
    {
    return 1;
    }
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  return (this->Number >= llist->NumberOfItems)? 1:0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkQueueIterator<DType>::GoToNextItem()
{
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  if(this->Number < llist->NumberOfItems)
    {
    this->Index = (this->Index + 1) % llist->GetSize();
    this->Number ++;
    }
  else
    {
    this->Index = llist->End;
    this->Number = 0;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkQueueIterator<DType>::GoToPreviousItem()
{
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  if(this->Number > 0)
    {
    this->Index = (this->Index - 1) % llist->GetSize();
    this->Number --;
    }
  else
    {
    this->Index = llist->Start;
    this->Number = llist->NumberOfItems-1;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkQueueIterator<DType>::GoToFirstItem()
{
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  this->Index = llist->End;
  this->Number = 0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkQueueIterator<DType>::GoToLastItem()
{
  vtkQueue<DType> *llist = static_cast<vtkQueue<DType>*>(this->Container);
  this->Index = llist->Start;  
  this->Number = llist->NumberOfItems-1;
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class DType>
vtkQueueIterator<DType>::vtkQueueIterator(const vtkQueueIterator<DType>&){}
template <class DType>
void vtkQueueIterator<DType>::operator=(const vtkQueueIterator<DType>&){}
#endif

#endif



