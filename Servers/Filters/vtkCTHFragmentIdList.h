/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentIdList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentIdList
// .SECTION Description
// Class that facilitates efficient operation on
// lists fragment ids. This class is introduced to 
// deal with the fact that local to global id search
// is a constant time operation, while its inverse
// glooabl to local id search is not.

#ifndef __vtkCTHFragmentIdList_h
#define __vtkCTHFragmentIdList_h

#include <vtkstd/vector>

class vtkCTHFragmentIdListItem;

class vtkCTHFragmentIdList
{
  public:
    vtkCTHFragmentIdList()
    {
      this->Clear();
    }
    ~vtkCTHFragmentIdList()
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
    void Initialize(vtkstd::vector<int> ids, bool preSorted=false);
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
    vtkstd::vector<vtkCTHFragmentIdListItem> IdList;
};

class vtkCTHFragmentIdListItem
{
  public:
    enum {LOCAL_ID=0, GLOBAL_ID=1, SIZE=2};
    //
    vtkCTHFragmentIdListItem()
    {
      this->Clear();
    }
    vtkCTHFragmentIdListItem(int globalId)
    {
      this->Initialize(-1,globalId);
    }
    //
    vtkCTHFragmentIdListItem(const vtkCTHFragmentIdListItem &other)
    {
      this->Initialize(other.GetLocalId(),other.GetGlobalId());
    }
    //
    ~vtkCTHFragmentIdListItem()
    {
      this->Clear();
    }
    //
    void Clear()
    {
      this->Data[LOCAL_ID]=-1;
      this->Data[GLOBAL_ID]=-1;
    }
    //
    void Initialize(int localId, int globalId)
    {
      this->Data[LOCAL_ID]=localId;
      this->Data[GLOBAL_ID]=globalId;
    }
    //
    int GetLocalId() const
    {
      return this->Data[LOCAL_ID];
    }
    //
    int GetGlobalId() const
    {
      return this->Data[GLOBAL_ID];
    }
    // Comparison made by global id.
    bool operator<(vtkCTHFragmentIdListItem &other)
    {
      return this->GetGlobalId()<other.GetGlobalId();
    }
    // Comparison made by global id.
    bool operator<=(vtkCTHFragmentIdListItem &other)
    {
      return this->GetGlobalId()<=other.GetGlobalId();
    }
    // Comparison made by global id.
    bool operator>(vtkCTHFragmentIdListItem &other)
    {
      return this->GetGlobalId()>other.GetGlobalId();
    }
    // Comparison made by global id.
    bool operator>=(vtkCTHFragmentIdListItem &other)
    {
      return this->GetGlobalId()>=other.GetGlobalId();
    }
    // Comparison made by global id.
    bool operator==(vtkCTHFragmentIdListItem &other)
    {
      return this->GetGlobalId()==other.GetGlobalId();
    }
    // 
    vtkCTHFragmentIdListItem &operator=(const vtkCTHFragmentIdListItem &other)
    {
      this->Initialize(other.GetLocalId(),other.GetGlobalId());
      return *this;
    }
  private:
    int Data[SIZE];
};

#endif
