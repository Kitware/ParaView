/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentIdListItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHFragmentIdListItem
// .SECTION Description
// Elemental data type for fragment id list conatiners.

#ifndef __vtkCTHFragmentIdListItem_h
#define __vtkCTHFragmentIdListItem_h


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
