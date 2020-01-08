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
#include "vtkMaterialInterfaceProcessRing.h"
#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;
//
class vtkMaterialInterfaceProcessRing::BufferContainer : public std::vector<vtkIdType>
{
};
//
vtkMaterialInterfaceProcessRing::vtkMaterialInterfaceProcessRing()
{
  this->NextElement = 0;
  this->BufferSize = 0;
  this->Buffer = new BufferContainer;
}
//
vtkMaterialInterfaceProcessRing::~vtkMaterialInterfaceProcessRing()
{
  delete this->Buffer;
}
//
void vtkMaterialInterfaceProcessRing::Clear()
{
  this->NextElement = 0;
  this->BufferSize = 0;
  this->Buffer->clear();
}
//
void vtkMaterialInterfaceProcessRing::Initialize(int nProcs)
{
  this->NextElement = 0;
  this->BufferSize = nProcs;
  this->Buffer->resize(nProcs);
  for (int procId = 0; procId < nProcs; ++procId)
  {
    (*this->Buffer)[procId] = procId;
  }
}
//
// void vtkMaterialInterfaceProcessRing::Initialize(
//     vtkMaterialInterfaceProcessPriorityQueue &Q,
//     vtkIdType upperLoadingBound)
// {
//   this->NextElement=0;
//   this->BufferSize=0;
//   this->Buffer->clear();
//
//   // check that upper bound does not exclude
//   // all processes.
//   vtkMaterialInterfaceProcessLoading &pl=Q.Pop();
//   vtkIdType minimumLoading=pl.GetLoadFactor();
//
//   // Make sure at least one process can be used.
//   if (upperLoadingBound!=-1
//       && minimumLoading>upperLoadingBound)
//     {
//     upperLoadingBound=minimumLoading;
//     cerr << "vtkMaterialInterfaceProcessRing "
//           << "[" << __LINE__ << "] "
//           << "Error: Upper loading bound excludes all processes."
//           << endl;
//     }
//
//   // Build ring of process ids.
//   this->Buffer->push_back(pl.GetId());
//   ++this->BufferSize;
//   while (!Q.Empty())
//     {
//     pl=Q.Pop();
//     if (upperLoadingBound!=-1
//         && pl.GetLoadFactor()>upperLoadingBound)
//       {
//       break;
//       }
//     this->Buffer->push_back(pl.GetId());
//     ++this->BufferSize;
//     }
// }
//
void vtkMaterialInterfaceProcessRing::Initialize(
  std::vector<vtkMaterialInterfaceProcessLoading>& Q, vtkIdType upperLoadingBound)
{
  this->NextElement = 0;
  this->BufferSize = 0;
  this->Buffer->clear();

  size_t nItems = Q.size();
  assert(nItems > 0);

  // check that upper bound does not exclude
  // all processes.
  vtkMaterialInterfaceProcessLoading& pl = Q[0];
  vtkIdType minimumLoading = pl.GetLoadFactor();

  // Make sure at least one process can be used.
  if (upperLoadingBound != -1 && minimumLoading > upperLoadingBound)
  {
    upperLoadingBound = minimumLoading;
    cerr << "vtkMaterialInterfaceProcessRing "
         << "[" << __LINE__ << "] "
         << "Error: Upper loading bound excludes all processes." << endl;
  }

  // Build ring of process ids.
  this->Buffer->push_back(pl.GetId());
  ++this->BufferSize;
  for (size_t itemId = 1; itemId < nItems; ++itemId)
  {
    pl = Q[itemId];
    if (upperLoadingBound != -1 && pl.GetLoadFactor() > upperLoadingBound)
    {
      break;
    }
    this->Buffer->push_back(pl.GetId());
    ++this->BufferSize;
  }
}
//
vtkIdType vtkMaterialInterfaceProcessRing::GetNextId()
{
  vtkIdType id = (*this->Buffer)[this->NextElement];
  ++this->NextElement;
  if (this->NextElement == this->BufferSize)
  {
    this->NextElement = 0;
  }
  return id;
}
//
void vtkMaterialInterfaceProcessRing::Print()
{
  size_t n = this->Buffer->size();
  if (n == 0)
  {
    cerr << "{}";
    return;
  }
  cerr << "{" << (*this->Buffer)[0];
  for (size_t i = 1; i < n; ++i)
  {
    cerr << ", " << (*this->Buffer)[i];
  }
  cerr << "}";
}
