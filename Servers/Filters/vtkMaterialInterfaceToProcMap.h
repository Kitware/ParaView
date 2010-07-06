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
// .NAME vtkMaterialInterfaceProcMap
// .SECTION Description
// Data structure which allows constant time determination
// of weather a given process has a piece of some fragment,
// and the number of processes which a specified fragment is
// spread across.

#ifndef __vtkMaterialInterfaceToProcMap_h
#define __vtkMaterialInterfaceToProcMap_h

#include "vtkstd/vector"

class vtkMaterialInterfaceToProcMap
{
public:
  vtkMaterialInterfaceToProcMap(){ this->Clear(); }
  vtkMaterialInterfaceToProcMap(int nFragments);
  vtkMaterialInterfaceToProcMap(int nProcs, int nFragments);
  vtkMaterialInterfaceToProcMap(const vtkMaterialInterfaceToProcMap &other);
  vtkMaterialInterfaceToProcMap &operator=(const vtkMaterialInterfaceToProcMap &rhs);
  // logistics
  void Clear();
  void Initialize(int nFragments);
  void Initialize(int nProcs, int nFragments);
  void DeepCopy(const vtkMaterialInterfaceToProcMap &from);
  // interface
  int GetProcOwnsPiece(int fragmentId) const;
  int GetProcOwnsPiece(int procId, int fragmentId) const;
  void SetProcOwnsPiece(int procId, int fragmentId);
  void SetProcOwnsPiece(int fragmentId);
  void UnSetProcOwnsPiece(int procId, int fragmentId);
  void UnSetProcOwnsPiece(int fragmentId);
  vtkstd::vector<int> WhoHasAPiece(int fragmentId, int excludeProc) const;
  vtkstd::vector<int> WhoHasAPiece(int fragmentId) const;
  int GetProcCount(int fragmentId){ return this->ProcCount[fragmentId]; }
private:
  // proc -> fragment -> bit mask, bit is 1 if
  // the fragment is on proc
  vtkstd::vector<vtkstd::vector<int> > PieceToProcMap;
  // fragment id -> count num procs
  vtkstd::vector<int> ProcCount;

  int NProcs;     // number of procs
  int NFragments; // number of fragments to map
  int PieceToProcMapSize; // length of map array
  int BitsPerInt; // number of bits in an integer
};
#endif
