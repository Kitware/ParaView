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
// .NAME vtkMaterialInterfacePieceLoading
// .SECTION Description
// Data structure that describes a fragment's loading.
// Holds its id and its loading factor.

#ifndef __vtkMaterialInterfacePieceLoading_h
#define __vtkMaterialInterfacePieceLoading_h

#include "vtkSystemIncludes.h"

#include <cassert>
#include "vtkType.h"
#include "vector"
#include "vtksys/ios/iostream"

class vtkMaterialInterfacePieceLoading
{
public:
  enum {ID=0,LOADING=1,SIZE=2};
  vtkMaterialInterfacePieceLoading(){ this->Initialize(-1,0); }
  ~vtkMaterialInterfacePieceLoading(){ this->Initialize(-1,0); }
  void Initialize(int id, vtkIdType loading)
  {
    this->Data[ID]=id;
    this->Data[LOADING]=loading;
  }
  // Description:
  // Place into a buffer (id, loading)
  void Pack(vtkIdType *buf)
  {
    buf[ID]=this->Data[ID];
    buf[LOADING]=this->Data[LOADING];
  }
  // Description:
  // Initialize from a buffer (id, loading)
  void UnPack(vtkIdType *buf)
  {
    this->Data[ID]=buf[ID];
    this->Data[LOADING]=buf[LOADING];
  }
  // Description:
  // Set/Get
  int GetId() const{ return this->Data[ID]; }
  vtkIdType GetLoading() const{ return this->Data[LOADING]; }
  void SetLoading(vtkIdType loading){ this->Data[LOADING]=loading; }
  // Description:
  // Adds to laoding and returns the updated loading.
  vtkIdType UpdateLoading(vtkIdType update)
  {
    assert("Update would make loading negative."
            && (this->Data[LOADING]+update)>=0);
    return this->Data[LOADING]+=update;
  }
  // Description:
  // Comparision are made by id.
  bool operator<(const vtkMaterialInterfacePieceLoading &other) const
  {
    return this->Data[ID]<other.Data[ID];
  }
  bool operator==(const vtkMaterialInterfacePieceLoading &other) const
  {
    return this->Data[ID]==other.Data[ID];
  }
private:
  vtkIdType Data[SIZE];
};
vtksys_ios::ostream &operator<<(vtksys_ios::ostream &sout, vtkMaterialInterfacePieceLoading &fp);
void PrintPieceLoadingHistogram(std::vector<std::vector<vtkIdType> > &pla);
#endif
