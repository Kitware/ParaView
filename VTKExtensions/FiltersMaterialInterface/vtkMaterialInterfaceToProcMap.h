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
 * @class   vtkMaterialInterfaceProcMap
 *
 * Data structure which allows constant time determination
 * of whether a given process has a piece of some fragment,
 * and the number of processes which a specified fragment is
 * spread across.
*/

#ifndef vtkMaterialInterfaceToProcMap_h
#define vtkMaterialInterfaceToProcMap_h

#include "vtkPVVTKExtensionsFiltersMaterialInterfaceModule.h" //needed for exports

#include <vector> // for WhoHasAPiece()

class VTKPVVTKEXTENSIONSFILTERSMATERIALINTERFACE_EXPORT vtkMaterialInterfaceToProcMap
{
public:
  vtkMaterialInterfaceToProcMap();
  vtkMaterialInterfaceToProcMap(int nFragments);
  vtkMaterialInterfaceToProcMap(int nProcs, int nFragments);
  vtkMaterialInterfaceToProcMap(const vtkMaterialInterfaceToProcMap& other);
  ~vtkMaterialInterfaceToProcMap();
  vtkMaterialInterfaceToProcMap& operator=(const vtkMaterialInterfaceToProcMap& rhs);
  // logistics
  void Clear();
  void Initialize(int nFragments);
  void Initialize(int nProcs, int nFragments);
  void DeepCopy(const vtkMaterialInterfaceToProcMap& from);
  // interface
  int GetProcOwnsPiece(int fragmentId) const;
  int GetProcOwnsPiece(int procId, int fragmentId) const;
  void SetProcOwnsPiece(int procId, int fragmentId);
  void SetProcOwnsPiece(int fragmentId);
  std::vector<int> WhoHasAPiece(int fragmentId, int excludeProc) const;
  std::vector<int> WhoHasAPiece(int fragmentId) const;
  int GetProcCount(int fragmentId);

private:
  // proc -> fragment -> bit mask, bit is 1 if
  // the fragment is on proc
  class PieceToProcMapContainer;
  PieceToProcMapContainer* PieceToProcMap;
  // fragment id -> count num procs
  class ProcCountContainer;
  ProcCountContainer* ProcCount;

  int NProcs;             // number of procs
  int NFragments;         // number of fragments to map
  int PieceToProcMapSize; // length of map array
  int BitsPerInt;         // number of bits in an integer
};
#endif

// VTK-HeaderTest-Exclude: vtkMaterialInterfaceToProcMap.h
