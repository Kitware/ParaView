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



