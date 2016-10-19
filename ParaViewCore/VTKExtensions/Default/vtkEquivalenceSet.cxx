/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEquivalenceSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEquivalenceSet.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkEquivalenceSet);

//============================================================================
// A class that implements an equivalent set.  It is used to combine fragments
// from different processes.
//
// I believe that this class is a strictly ordered tree of equivalences.
// Every member points to its own id or an id smaller than itself.

//----------------------------------------------------------------------------
vtkEquivalenceSet::vtkEquivalenceSet()
{
  this->Resolved = 0;
  this->EquivalenceArray = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkEquivalenceSet::~vtkEquivalenceSet()
{
  this->Resolved = 0;
  if (this->EquivalenceArray)
  {
    this->EquivalenceArray->Delete();
    this->EquivalenceArray = 0;
  }
}

//----------------------------------------------------------------------------
void vtkEquivalenceSet::Initialize()
{
  this->Resolved = 0;
  this->NumberOfResolvedSets = 0;
  this->EquivalenceArray->Initialize();
}

//----------------------------------------------------------------------------
void vtkEquivalenceSet::DeepCopy(vtkEquivalenceSet* in)
{
  this->Resolved = in->Resolved;
  this->EquivalenceArray->DeepCopy(in->EquivalenceArray);
}

//----------------------------------------------------------------------------
void vtkEquivalenceSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  vtkIdType num = this->GetNumberOfMembers();
  os << indent << "NumberOfMembers: " << num << endl;
  for (vtkIdType ii = 0; ii < num; ++ii)
  {
    os << indent << "  " << ii << ": " << this->GetEquivalentSetId(ii) << endl;
  }
  os << endl;
}

//----------------------------------------------------------------------------
// Return the id of the equivalent set.
int vtkEquivalenceSet::GetEquivalentSetId(int memberId)
{
  int ref;

  ref = this->GetReference(memberId);
  while (!this->Resolved && ref != memberId)
  {
    memberId = ref;
    ref = this->GetReference(memberId);
  }

  return ref;
}

//----------------------------------------------------------------------------
// Return the id of the equivalent set.
int vtkEquivalenceSet::GetReference(int memberId)
{
  if (memberId >= this->EquivalenceArray->GetNumberOfTuples())
  { // We might consider this an error ...
    return memberId;
  }
  return this->EquivalenceArray->GetValue(memberId);
}

//----------------------------------------------------------------------------
// Makes two new or existing ids equivalent.
// If the array is too small, the range of ids is increased until it contains
// both the ids.  Negative ids are not allowed.
void vtkEquivalenceSet::AddEquivalence(int id1, int id2)
{
  if (this->Resolved)
  {
    vtkGenericWarningMacro("Set already resolved, you cannot add more equivalences.");
    return;
  }

  int num = this->EquivalenceArray->GetNumberOfTuples();

  // Expand the range to include both ids.
  while (num <= id1 || num <= id2)
  {
    // All values inserted are equivalent to only themselves.
    this->EquivalenceArray->InsertNextTuple1(num);
    ++num;
  }

  // Our rule for references in the equivalent set is that
  // all elements must point to a member equal to or smaller
  // than itself.

  // The only problem we could encounter is changing a reference.
  // we do not want to orphan anything previously referenced.

  // Replace the larger references.
  if (id1 < id2)
  {
    // We could follow the references to the smallest.  It might
    // make this processing more efficient.  This is a compromise.
    // The referenced id will always be smaller than the id so
    // order does not change.
    this->EquateInternal(this->GetReference(id1), id2);
  }
  else
  {
    this->EquateInternal(this->GetReference(id2), id1);
  }
}

//----------------------------------------------------------------------------
int vtkEquivalenceSet::GetNumberOfMembers()
{
  return this->EquivalenceArray->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
int* vtkEquivalenceSet::GetPointer()
{
  return this->EquivalenceArray->GetPointer(0);
}

//----------------------------------------------------------------------------
void vtkEquivalenceSet::Squeeze()
{
  this->EquivalenceArray->Squeeze();
}

//----------------------------------------------------------------------------
vtkIdType vtkEquivalenceSet::Capacity()
{
  return this->EquivalenceArray->Capacity();
}

//----------------------------------------------------------------------------
// id1 must be less than or equal to id2.
void vtkEquivalenceSet::EquateInternal(int id1, int id2)
{
  // This is the reference that might be orphaned in this process.
  int oldRef = this->GetEquivalentSetId(id2);

  // The two ids are already equal (not the only way they might be equal).
  if (oldRef == id1)
  {
    return;
  }

  // The only problem we could encounter is changing a reference.
  // we do not want to orphan anything previously referenced.
  if (oldRef == id2)
  {
    this->EquivalenceArray->SetValue(id2, id1);
  }
  else if (oldRef > id1)
  {
    this->EquivalenceArray->SetValue(id2, id1);
    this->EquateInternal(id1, oldRef);
  }
  else
  { // oldRef < id1
    this->EquateInternal(oldRef, id1);
  }
}

//----------------------------------------------------------------------------
// Returns the number of merged sets.
int vtkEquivalenceSet::ResolveEquivalences()
{
  // Go through the equivalence array collapsing chains
  // and assigning consecutive ids.
  int count = 0;
  int id;
  int newId;

  int numIds = this->EquivalenceArray->GetNumberOfTuples();
  for (int ii = 0; ii < numIds; ++ii)
  {
    id = this->EquivalenceArray->GetValue(ii);
    if (id == ii)
    { // This is a new equivalence set.
      this->EquivalenceArray->SetValue(ii, count);
      ++count;
    }
    else
    {
      // All earlier ids will be resolved already.
      // This array only point to less than or equal ids. (id <= ii).
      newId = this->EquivalenceArray->GetValue(id);
      this->EquivalenceArray->SetValue(ii, newId);
    }
  }
  this->Resolved = 1;
  // cerr << "Final number of equivalent sets: " << count << endl;

  this->NumberOfResolvedSets = count;

  return count;
}
