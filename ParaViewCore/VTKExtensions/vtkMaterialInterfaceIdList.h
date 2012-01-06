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
// .NAME vtkMaterialInterfaceIdList
// .SECTION Description
// Class that facilitates efficient operation on
// lists fragment ids. This class is introduced to
// deal with the fact that local to global id search
// is a constant time operation, while its inverse
// glooabl to local id search is not.

#ifndef __vtkMaterialInterfaceIdList_h
#define __vtkMaterialInterfaceIdList_h

#include <vector>
#include "vtkMaterialInterfaceIdListItem.h"

class vtkMaterialInterfaceIdList
{
public:
  vtkMaterialInterfaceIdList()
  {
    this->Clear();
  }
  ~vtkMaterialInterfaceIdList()
  {
    this->Clear();
  }
  // Description:
  // Return the container to an empty state.
  void Clear()
  {
    this->IdList.clear();
    this->IsInitialized=false;
  }
  // Description:
  // Initialize the container with a list of id's
  // these must be in ascending order.
  void Initialize(std::vector<int> ids, bool preSorted=false);
  // Description:
  // Initialize the container from the ids that have
  // been pushed. This must be done before querrying.
  void Initialize();
  // Description:
  // Add a fragment id to the list. Return its index.
  int PushId();
  // Description:
  // Add a fragment id to the list, if it is unique.
  // Return its index or -1 to indicate its not unique.
  int PushUniqueId();
  // Description:
  // Given a global id, get the local id, or -1 if the
  // global id is not in the list.
  int GetLocalId(int globalId);
private:
  bool IsInitialized;
  std::vector<vtkMaterialInterfaceIdListItem> IdList;
};
#endif
