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
// Include blockers needed since vtkArrayMap.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkArrayMapIterator_txx
#define __vtkArrayMapIterator_txx

#include "vtkArrayMapIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkArrayMap.h"

template <class KeyType,class DataType>
vtkArrayMapIterator<KeyType,DataType> *vtkArrayMapIterator<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkArrayMapIterator");
#endif
  return new vtkArrayMapIterator<KeyType,DataType>(); 
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::InitTraversal()
{
  this->GoToFirstItem();
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GetKey(KeyType& key)
{  
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  vtkAbstractMapItem<KeyType,DataType> *item = 0;
  if ( !lmap || lmap->Array->GetItem(this->Index, item) != VTK_OK )
    {
    return VTK_ERROR;
    }
  key = item->Key;
  return VTK_OK;
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::GetData(DataType& data)
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  vtkAbstractMapItem<KeyType,DataType> *item = 0;
  if ( !lmap || lmap->Array->GetItem(this->Index, item) != VTK_OK )
    {
    return VTK_ERROR;
    }
  data = item->Data;
  return VTK_OK;
}

template<class KeyType,class DataType>
int vtkArrayMapIterator<KeyType,DataType>::IsDoneWithTraversal()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if ( !lmap || this->Index < 0 || this->Index >= lmap->GetNumberOfItems() )
    {
    return 1;
    }
  return 0;
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToNextItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(this->Index < lmap->GetNumberOfItems())
    {
    ++this->Index;
    }
  else
    {
    this->Index = 0;
    }
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToPreviousItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(this->Index > 0)
    {
    --this->Index;
    }
  else
    {
    this->Index = lmap->GetNumberOfItems();
    }
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToFirstItem()
{
  this->Index = 0;
}

template<class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::GoToLastItem()
{
  vtkArrayMap<KeyType,DataType> *lmap 
    = static_cast<vtkArrayMap<KeyType,DataType>*>(this->Container);
  if(lmap->GetNumberOfItems() > 0)
    {
    this->Index = lmap->GetNumberOfItems()-1;  
    }
  else
    {
    this->Index = 0;
    }
}

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkArrayMapIterator<KeyType,DataType>::vtkArrayMapIterator(const vtkArrayMapIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkArrayMapIterator<KeyType,DataType>::operator=(const vtkArrayMapIterator<KeyType,DataType>&){}
#endif

#endif



