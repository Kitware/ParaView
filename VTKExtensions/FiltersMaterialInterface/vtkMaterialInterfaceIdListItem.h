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
/**
 * @class   vtkMaterialInterfaceIdListItem
 *
 * Elemental data type for fragment id list containers.
*/

#ifndef vtkMaterialInterfaceIdListItem_h
#define vtkMaterialInterfaceIdListItem_h

#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceIdListItem
{
public:
  enum
  {
    LOCAL_ID = 0,
    GLOBAL_ID = 1,
    SIZE = 2
  };
  //
  vtkMaterialInterfaceIdListItem() { this->Clear(); }
  vtkMaterialInterfaceIdListItem(int globalId) { this->Initialize(-1, globalId); }
  //
  vtkMaterialInterfaceIdListItem(const vtkMaterialInterfaceIdListItem& other)
  {
    this->Initialize(other.GetLocalId(), other.GetGlobalId());
  }
  //
  ~vtkMaterialInterfaceIdListItem() { this->Clear(); }
  //
  void Clear()
  {
    this->Data[LOCAL_ID] = -1;
    this->Data[GLOBAL_ID] = -1;
  }
  //
  void Initialize(int localId, int globalId)
  {
    this->Data[LOCAL_ID] = localId;
    this->Data[GLOBAL_ID] = globalId;
  }
  //
  int GetLocalId() const { return this->Data[LOCAL_ID]; }
  //
  int GetGlobalId() const { return this->Data[GLOBAL_ID]; }
  // Comparison made by global id.
  bool operator<(const vtkMaterialInterfaceIdListItem& other) const
  {
    return this->GetGlobalId() < other.GetGlobalId();
  }
  // Comparison made by global id.
  bool operator<=(const vtkMaterialInterfaceIdListItem& other) const
  {
    return this->GetGlobalId() <= other.GetGlobalId();
  }
  // Comparison made by global id.
  bool operator>(const vtkMaterialInterfaceIdListItem& other) const
  {
    return this->GetGlobalId() > other.GetGlobalId();
  }
  // Comparison made by global id.
  bool operator>=(const vtkMaterialInterfaceIdListItem& other) const
  {
    return this->GetGlobalId() >= other.GetGlobalId();
  }
  // Comparison made by global id.
  bool operator==(const vtkMaterialInterfaceIdListItem& other) const
  {
    return this->GetGlobalId() == other.GetGlobalId();
  }
  //
  vtkMaterialInterfaceIdListItem& operator=(const vtkMaterialInterfaceIdListItem& other)
  {
    this->Initialize(other.GetLocalId(), other.GetGlobalId());
    return *this;
  }

private:
  int Data[SIZE];
};
#endif

// VTK-HeaderTest-Exclude: vtkMaterialInterfaceIdListItem.h
