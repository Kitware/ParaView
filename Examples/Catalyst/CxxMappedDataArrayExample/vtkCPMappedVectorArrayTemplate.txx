/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPMappedVectorArrayTemplate.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPMappedVectorArrayTemplate.h"

#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkVariant.h"
#include "vtkVariantCast.h"

//------------------------------------------------------------------------------
// Can't use vtkStandardNewMacro with a template.
template <class Scalar>
vtkCPMappedVectorArrayTemplate<Scalar>* vtkCPMappedVectorArrayTemplate<Scalar>::New()
{
  VTK_STANDARD_NEW_BODY(vtkCPMappedVectorArrayTemplate<Scalar>);
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkCPMappedVectorArrayTemplate<Scalar>::Superclass::PrintSelf(os, indent);
  os << indent << "Array: " << this->Array << std::endl;
  os << indent << "TempDoubleArray: " << this->TempDoubleArray << std::endl;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::Initialize()
{
  this->Array = NULL;
  this->MaxId = -1;
  this->Size = 0;
  this->NumberOfComponents = 3;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::GetTuples(vtkIdList* ptIds, vtkAbstractArray* output)
{
  vtkDataArray* outArray = vtkDataArray::FastDownCast(output);
  if (!outArray)
  {
    vtkWarningMacro(<< "Input is not a vtkDataArray");
    return;
  }

  vtkIdType numTuples = ptIds->GetNumberOfIds();

  outArray->SetNumberOfComponents(this->NumberOfComponents);
  outArray->SetNumberOfTuples(numTuples);

  const vtkIdType numPoints = ptIds->GetNumberOfIds();
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    outArray->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::GetTuples(
  vtkIdType p1, vtkIdType p2, vtkAbstractArray* output)
{
  vtkDataArray* da = vtkDataArray::FastDownCast(output);
  if (!da)
  {
    vtkErrorMacro(<< "Input is not a vtkDataArray");
    return;
  }

  if (da->GetNumberOfComponents() != this->GetNumberOfComponents())
  {
    vtkErrorMacro(<< "Incorrect number of components in input array.");
    return;
  }

  for (vtkIdType daTupleId = 0; p1 <= p2; ++p1)
  {
    da->SetTuple(daTupleId++, this->GetTuple(p1));
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::Squeeze()
{
  // noop
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkArrayIterator* vtkCPMappedVectorArrayTemplate<Scalar>::NewIterator()
{
  vtkErrorMacro(<< "Not implemented.");
  return NULL;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::LookupValue(vtkVariant value)
{
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  if (valid)
  {
    return this->Lookup(val, 0);
  }
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::LookupValue(vtkVariant value, vtkIdList* ids)
{
  bool valid = true;
  Scalar val = vtkVariantCast<Scalar>(value, &valid);
  ids->Reset();
  if (valid)
  {
    vtkIdType index = 0;
    while ((index = this->Lookup(val, index)) >= 0)
    {
      ids->InsertNextId(index++);
    }
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkVariant vtkCPMappedVectorArrayTemplate<Scalar>::GetVariantValue(vtkIdType idx)
{
  return vtkVariant(this->GetValueReference(idx));
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::ClearLookup()
{
  // no-op, no fast lookup implemented.
}

//------------------------------------------------------------------------------
template <class Scalar>
double* vtkCPMappedVectorArrayTemplate<Scalar>::GetTuple(vtkIdType i)
{
  this->GetTuple(i, this->TempDoubleArray);
  return this->TempDoubleArray;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::GetTuple(vtkIdType i, double* tuple)
{
  tuple[0] = static_cast<double>(this->Array[i]);
  tuple[1] = static_cast<double>(this->Array[i + this->GetNumberOfTuples()]);
  tuple[2] = static_cast<double>(this->Array[i + 2 * this->GetNumberOfTuples()]);
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::LookupTypedValue(Scalar value)
{
  return this->Lookup(value, 0);
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::LookupTypedValue(Scalar value, vtkIdList* ids)
{
  ids->Reset();
  vtkIdType index = 0;
  while ((index = this->Lookup(value, index)) >= 0)
  {
    ids->InsertNextId(index++);
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
typename vtkCPMappedVectorArrayTemplate<Scalar>::ValueType
vtkCPMappedVectorArrayTemplate<Scalar>::GetValue(vtkIdType idx) const
{
  // Work around const-correct inconsistencies:
  typedef vtkCPMappedVectorArrayTemplate<Scalar> ThisClass;
  return const_cast<ThisClass*>(this)->GetValueReference(idx);
}

//------------------------------------------------------------------------------
template <class Scalar>
Scalar& vtkCPMappedVectorArrayTemplate<Scalar>::GetValueReference(vtkIdType idx)
{
  const vtkIdType tuple = idx / this->NumberOfComponents;
  const vtkIdType comp = idx % this->NumberOfComponents;
  switch (comp)
  {
    case 0:
      return this->Array[tuple];
    case 1:
      return this->Array[tuple + this->GetNumberOfTuples()];
    case 2:
      return this->Array[tuple + 2 * this->GetNumberOfTuples()];
    default:
      vtkErrorMacro(<< "Invalid number of components.");
      static Scalar dummy(0);
      return dummy;
  }
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::GetTypedTuple(vtkIdType tupleId, Scalar* tuple) const
{
  // Work around const-correct inconsistencies:
  typedef vtkCPMappedVectorArrayTemplate<Scalar> ThisClass;
  vtkIdType numTuples = const_cast<ThisClass*>(this)->GetNumberOfTuples();

  tuple[0] = this->Array[tupleId];
  tuple[1] = this->Array[tupleId + numTuples];
  tuple[2] = this->Array[tupleId + 2 * numTuples];
}

//------------------------------------------------------------------------------
template <class Scalar>
int vtkCPMappedVectorArrayTemplate<Scalar>::Allocate(vtkIdType, vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
int vtkCPMappedVectorArrayTemplate<Scalar>::Resize(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return 0;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetNumberOfTuples(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetTuple(vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetTuple(vtkIdType, const float*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetTuple(vtkIdType, const double*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTuple(vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTuple(vtkIdType, const float*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTuple(vtkIdType, const double*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTuples(vtkIdList*, vtkIdList*, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTuples(
  vtkIdType, vtkIdType, vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::InsertNextTuple(vtkIdType, vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::InsertNextTuple(const float*)
{

  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::InsertNextTuple(const double*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::DeepCopy(vtkAbstractArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::DeepCopy(vtkDataArray*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InterpolateTuple(
  vtkIdType, vtkIdList*, vtkAbstractArray*, double*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InterpolateTuple(
  vtkIdType, vtkIdType, vtkAbstractArray*, vtkIdType, vtkAbstractArray*, double)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertVariantValue(vtkIdType, vtkVariant)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::RemoveTuple(vtkIdType)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::RemoveFirstTuple()
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::RemoveLastTuple()
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertTypedTuple(vtkIdType, const Scalar*)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::InsertNextTypedTuple(const Scalar*)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::InsertNextValue(Scalar)
{
  vtkErrorMacro("Read only container.");
  return -1;
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::InsertValue(vtkIdType, Scalar)
{
  vtkErrorMacro("Read only container.");
  return;
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkCPMappedVectorArrayTemplate<Scalar>::vtkCPMappedVectorArrayTemplate()
  : Array(NULL)
{
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkCPMappedVectorArrayTemplate<Scalar>::~vtkCPMappedVectorArrayTemplate()
{
}

//------------------------------------------------------------------------------
template <class Scalar>
void vtkCPMappedVectorArrayTemplate<Scalar>::SetVectorArray(Scalar* array, vtkIdType numPoints)
{
  Initialize();
  this->Array = array;
  this->NumberOfComponents = 3;
  this->Size = this->NumberOfComponents * numPoints;
  this->MaxId = this->Size - 1;
  this->Modified();
}

//------------------------------------------------------------------------------
template <class Scalar>
vtkIdType vtkCPMappedVectorArrayTemplate<Scalar>::Lookup(const Scalar& val, vtkIdType index)
{
  while (index <= this->MaxId)
  {
    if (this->GetValueReference(index++) == val)
    {
      return index;
    }
  }
  return -1;
}
