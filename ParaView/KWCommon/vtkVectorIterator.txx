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

#ifndef __vtkVectorIterator_txx
#define __vtkVectorIterator_txx

#include "vtkVectorIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkVector.h"

//----------------------------------------------------------------------------
template <class DType>
vtkVectorIterator<DType> *vtkVectorIterator<DType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkVectorIterator");
#endif
  return new vtkVectorIterator<DType>(); 
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::InitTraversal()
{
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::GetKey(vtkIdType& key)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  key = this->Index;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::GetData(DType& data)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  data = llist->Array[this->Index];
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::SetData(const DType& data)
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if ( this->Index == llist->NumberOfItems ) { return VTK_ERROR; }
  llist->Array[this->Index] = data;
  return VTK_OK;
}

//----------------------------------------------------------------------------
template<class DType>
int vtkVectorIterator<DType>::IsDoneWithTraversal()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  return (this->Index == llist->NumberOfItems)? 1:0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToNextItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(this->Index < llist->NumberOfItems)
    {
    ++this->Index;
    }
  else
    {
    this->Index = 0;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToPreviousItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(this->Index > 0)
    {
    --this->Index;
    }
  else
    {
    this->Index = llist->NumberOfItems;
    }
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToFirstItem()
{
  this->Index = 0;
}

//----------------------------------------------------------------------------
template<class DType>
void vtkVectorIterator<DType>::GoToLastItem()
{
  vtkVector<DType> *llist = static_cast<vtkVector<DType>*>(this->Container);
  if(llist->NumberOfItems > 0)
    {
    this->Index = llist->NumberOfItems-1;  
    }
  else
    {
    this->Index = 0;
    }
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class DType>
vtkVectorIterator<DType>::vtkVectorIterator(const vtkVectorIterator<DType>&){}
template <class DType>
void vtkVectorIterator<DType>::operator=(const vtkVectorIterator<DType>&){}
#endif

#endif



