/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMaterialInterfaceIdList.h"
#include <algorithm>
#include <cassert>

namespace
{
// Binary search. l & r are the range to search in.
int search(
  vtkMaterialInterfaceIdListItem* idList, int l, int r, vtkMaterialInterfaceIdListItem& itemToFind)
{
  assert(l <= r);
  int m = (r + l) / 2;
  // found it.
  if (idList[m] == itemToFind)
  {
    return idList[m].GetLocalId();
  }
  // If here it's in lower half.
  else if (itemToFind >= idList[l] && itemToFind < idList[m])
  {
    return search(idList, l, m - 1, itemToFind);
  }
  // If here it's in upper half.
  else if (itemToFind > idList[m] && itemToFind <= idList[r])
  {
    return search(idList, m + 1, r, itemToFind);
  }
  // It's not here.
  else
  {
    return -1;
  }
}
}

// PIMPL class
class vtkMaterialInterfaceIdList::IdListContainer
  : public std::vector<vtkMaterialInterfaceIdListItem>
{
};

vtkMaterialInterfaceIdList::vtkMaterialInterfaceIdList()
{
  this->IsInitialized = false;
  this->IdList = new IdListContainer;
}
//
vtkMaterialInterfaceIdList::~vtkMaterialInterfaceIdList()
{
  delete this->IdList;
}
//
void vtkMaterialInterfaceIdList::Clear()
{
  this->IdList->clear();
  this->IsInitialized = false;
}
//
void vtkMaterialInterfaceIdList::Initialize(const std::vector<int>& ids, bool preSorted)
{
  // Prep.
  this->Clear();
  int nLocalIds = static_cast<int>(ids.size());
  // Anything to do?
  if (nLocalIds < 1)
  {
    return;
  }
  // Make a copy of incoming, convert to list items to track
  // local id as items are sorted.
  this->IdList->resize(nLocalIds);
  for (int localId = 0; localId < nLocalIds; ++localId)
  {
    int globalId = ids[localId];
    (*this->IdList)[localId].Initialize(localId, globalId);
  }
  // Sort incoming items to support efficient searching.
  if (!preSorted)
  {
    // Heap sort, because these are likely to be, if not completely
    // in order almost in order.
    partial_sort(this->IdList->begin(), this->IdList->end(), this->IdList->end());
  }
  this->IsInitialized = true;
}
//
int vtkMaterialInterfaceIdList::GetLocalId(int globalId)
{
  assert("The object must be initialized before queries are made." && this->IsInitialized);

  const int firstListItem = 0;
  const int lastListItem = static_cast<int>(this->IdList->size()) - 1;
  vtkMaterialInterfaceIdListItem itemToFind(globalId);
  return search(&(*this->IdList)[0], firstListItem, lastListItem, itemToFind);
}
