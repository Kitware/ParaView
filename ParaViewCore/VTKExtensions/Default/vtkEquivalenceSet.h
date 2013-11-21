/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEquivalenceSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEquivalenceSet - records groups of integers that are equivalent.
// .SECTION Description
// Useful for connectivity on multiple processes.  Run connectivity
// on each processes, then make touching fragments equivalent.

#ifndef __vtkEquivalenceSet_h
#define __vtkEquivalenceSet_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkObject.h"
class vtkIntArray;


class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkEquivalenceSet : public vtkObject
{
public:
  vtkTypeMacro(vtkEquivalenceSet,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkEquivalenceSet *New();
  
  void Initialize();
  void AddEquivalence(int id1, int id2);

  // The length of the equivalent array...
  // The Domain of the equivalance map is [0, numberOfMembers).
  int GetNumberOfMembers();

  // Valid only after set is resolved.
  // The range of the map is [0 numberOfResolvedSets)
  int GetNumberOfResolvedSets() { return this->NumberOfResolvedSets;}


  // Return the id of the equivalent set.
  int GetEquivalentSetId(int memberId);

  // Equivalent set ids are reassinged to be sequential.
  // You cannot add anymore equivalences after this is called.
  virtual int ResolveEquivalences();

  void DeepCopy(vtkEquivalenceSet* in);

  // Needed for sending the set over MPI.
  // Be very careful with the pointer.  
  // I guess this means do not write to the memory.
  int* GetPointer();

  // Free unused memory
  void Squeeze();

  // Report used memory
  vtkIdType Capacity();

  // We should fix the pointer API and hide this ivar.
  int Resolved;

  int GetReference(int memberId);
protected:
  vtkEquivalenceSet();
  ~vtkEquivalenceSet();

  int NumberOfResolvedSets;

  // To merge connected framgments that have different ids because they were
  // traversed by different processes or passes.
  vtkIntArray *EquivalenceArray;

  // Return the id of the equivalent set.
  void EquateInternal(int id1, int id2);

private:
  vtkEquivalenceSet(const vtkEquivalenceSet&);  // Not implemented.
  void operator=(const vtkEquivalenceSet&);  // Not implemented.
};

#endif
