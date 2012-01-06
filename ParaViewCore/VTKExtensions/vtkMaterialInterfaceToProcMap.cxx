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

#include "vtkMaterialInterfaceToProcMap.h"
#include <cassert>
#include "vector"
using std::vector;

//
vtkMaterialInterfaceToProcMap::vtkMaterialInterfaceToProcMap(
              int nFragments)
{
  this->Initialize(nFragments);
}
//
vtkMaterialInterfaceToProcMap::vtkMaterialInterfaceToProcMap(
              int nProcs,
              int nFragments)
{
  this->Initialize(nProcs, nFragments);
}
//
vtkMaterialInterfaceToProcMap::vtkMaterialInterfaceToProcMap(
              const vtkMaterialInterfaceToProcMap &other)
{
  this->DeepCopy(other);
}
//
void vtkMaterialInterfaceToProcMap::Clear()
{
  this->PieceToProcMap.clear();
  this->ProcCount.clear();
  this->NProcs=0;
  this->NFragments=0;
  this->PieceToProcMapSize=0;
  this->BitsPerInt=0;
}
//
void vtkMaterialInterfaceToProcMap::Initialize(
              int nFragments)
{
  this->Initialize(1,nFragments);
}
//
void vtkMaterialInterfaceToProcMap::Initialize(
              int nProcs,
              int nFragments)
{
  this->Clear();

  this->NProcs=nProcs;
  this->NFragments=nFragments;
  this->BitsPerInt=8*sizeof(int);
  this->PieceToProcMapSize=nFragments/this->BitsPerInt+1;

  this->ProcCount.resize(nFragments,0);

  this->PieceToProcMap.resize(nProcs);
  for (int i=0; i<nProcs; ++i)
    {
    this->PieceToProcMap[i].resize(this->PieceToProcMapSize,0);
    }
}
//
vtkMaterialInterfaceToProcMap &vtkMaterialInterfaceToProcMap::operator=(
              const vtkMaterialInterfaceToProcMap &other)
{
  this->DeepCopy(other);

  return *this;
}
//
void vtkMaterialInterfaceToProcMap::DeepCopy(
              const vtkMaterialInterfaceToProcMap &from)
{
  this->NProcs=from.NProcs;
  this->NFragments=from.NFragments;
  this->PieceToProcMapSize=from.PieceToProcMapSize;
  this->BitsPerInt=from.BitsPerInt;
  this->PieceToProcMap=from.PieceToProcMap;
}
//
int vtkMaterialInterfaceToProcMap::GetProcOwnsPiece(
                int fragmentId) const
{
  return this->GetProcOwnsPiece(0,fragmentId);
}
//
int vtkMaterialInterfaceToProcMap::GetProcOwnsPiece(
                int procId,
                int fragmentId) const
{
  assert( "Invalid fragment id"
          && fragmentId >= 0
          && fragmentId < this->NFragments );
  assert( "Invalid proc id"
          && procId >= 0
          && procId < this->NProcs );

  int maskIdx=fragmentId/this->BitsPerInt;
  int maskBit=1<<fragmentId%this->BitsPerInt;

  return maskBit & this->PieceToProcMap[procId][maskIdx];
}
//
void vtkMaterialInterfaceToProcMap::SetProcOwnsPiece(int fragmentId)
{
  this->SetProcOwnsPiece(0,fragmentId);
}
//
void vtkMaterialInterfaceToProcMap::SetProcOwnsPiece(int procId, int fragmentId)
{
  assert( "Invalid fragment id"
          && fragmentId >= 0
          && fragmentId < this->NFragments );
  assert( "Invalid proc id"
          && procId >= 0
          && procId < this->NProcs );

  // set bit in this proc's mask array
  int maskIdx=fragmentId/this->BitsPerInt;
  int maskBit=1<<fragmentId%this->BitsPerInt;
  this->PieceToProcMap[procId][maskIdx] |= maskBit;

  // inc fragments ownership count
  ++this->ProcCount[fragmentId];
}
//
vector<int> vtkMaterialInterfaceToProcMap::WhoHasAPiece(
                int fragmentId,
                int excludeProc) const
{
  assert( "Invalid proc id"
          && excludeProc >= 0
          && excludeProc < this->NProcs );

  vector<int> whoHasList;

  for (int procId=0; procId<this->NProcs; ++procId)
    {
    if ( procId==excludeProc )
      {
      continue;
      }
    int maskIdx=fragmentId/this->BitsPerInt;
    int maskBit=1<<fragmentId%this->BitsPerInt;

    // this guy has a piece
    if (maskBit & this->PieceToProcMap[procId][maskIdx])
      {
      whoHasList.push_back(procId);
      }
    }
  return whoHasList;
}
//
vector<int> vtkMaterialInterfaceToProcMap::WhoHasAPiece(
                int fragmentId) const
{
  vector<int> whoHasList;

  for (int procId=0; procId<this->NProcs; ++procId)
    {
     int maskIdx=fragmentId/this->BitsPerInt;
     int maskBit=1<<fragmentId%this->BitsPerInt;

    // this guy has a piece
    if (maskBit & this->PieceToProcMap[procId][maskIdx])
      {
      whoHasList.push_back(procId);
      }
    }

  return whoHasList;
}
