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
// Include blockers needed since vtkHashMap.h includes this file
// when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkHashMapIterator_txx
#define __vtkHashMapIterator_txx

#include "vtkHashMapIterator.h"
#include "vtkAbstractIterator.txx"
#include "vtkHashMap.h"

//----------------------------------------------------------------------------
template <class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType> *vtkHashMapIterator<KeyType,DataType>::New()
{ 
#ifdef VTK_DEBUG_LEAKS
  vtkDebugLeaks::ConstructClass("vtkHashMapIterator");
#endif
  return new vtkHashMapIterator<KeyType,DataType>(); 
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::InitTraversal()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  if(!this->Iterator)
    {
    this->Iterator = hmap->Buckets[0]->NewIterator();
    }
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetKey(KeyType& key)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item = { KeyType(), DataType() };
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    key = item.Key;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetData(DataType& data)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item = { KeyType(), DataType() };
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    data = item.Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::GetKeyAndData(KeyType& key,
                                                        DataType& data)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item;
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    key = item.Key;
    data = item.Data;
    return VTK_OK;
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::SetData(const DataType& data)
{
  if(this->IsDoneWithTraversal()) { return VTK_ERROR; }
  ItemType item;
  if(this->Iterator->GetData(item) == VTK_OK)
    {
    item.Data = data;
    return this->Iterator->SetData(item);
    }
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
int vtkHashMapIterator<KeyType,DataType>::IsDoneWithTraversal()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  return (this->Bucket == hmap->NumberOfBuckets) ? 1:0;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToNextItem()
{
  if(this->IsDoneWithTraversal()) { this->GoToFirstItem(); }
  this->Iterator->GoToNextItem();
  this->ScanForward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToPreviousItem()
{
  if(this->IsDoneWithTraversal())
    {
    this->GoToLastItem();
    }
  else
    {
    this->Iterator->GoToPreviousItem();
    this->ScanBackward();
    }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToFirstItem()
{
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  this->Bucket = 0;
  this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
  this->Iterator->GoToFirstItem();
  this->ScanForward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::GoToLastItem()
{
  vtkHashMap<KeyType,DataType>* hmap 
  = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  this->Bucket = hmap->NumberOfBuckets-1;
  this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
  this->Iterator->GoToLastItem();
  this->ScanBackward();
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::vtkHashMapIterator()
{
  this->Iterator = 0;
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::~vtkHashMapIterator()
{
  if(this->Iterator) { this->Iterator->Delete(); }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::ScanForward()
{
  // Move the iterator forward until a valid item is reached.
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  
  while(this->Iterator->IsDoneWithTraversal() &&
        (++this->Bucket < hmap->NumberOfBuckets))
    {
    this->Iterator->SetContainer(hmap->Buckets[this->Bucket]);
    this->Iterator->GoToFirstItem();
    }
}

//----------------------------------------------------------------------------
template<class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::ScanBackward()
{
  // Move the iterator backward until a valid item is reached.
  vtkHashMap<KeyType,DataType>* hmap 
    = static_cast<vtkHashMap<KeyType,DataType>*>(this->Container);
  
  while(this->Iterator->IsDoneWithTraversal() &&
        (this->Bucket > 0))
    {
    this->Iterator->SetContainer(hmap->Buckets[--this->Bucket]);
    this->Iterator->GoToLastItem();
    }
  
  // If no valid item was reached, indicate that the traversal is done.
  if(this->Iterator->IsDoneWithTraversal())
    {
    this->Bucket = hmap->NumberOfBuckets;
    }
}

//----------------------------------------------------------------------------

#if defined ( _MSC_VER )
template <class KeyType,class DataType>
vtkHashMapIterator<KeyType,DataType>::vtkHashMapIterator(const vtkHashMapIterator<KeyType,DataType>&){}
template <class KeyType,class DataType>
void vtkHashMapIterator<KeyType,DataType>::operator=(const vtkHashMapIterator<KeyType,DataType>&){}
#endif

#endif



