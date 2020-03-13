/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPMappedVectorArrayTemplate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkCPMappedVectorArrayTemplate - Map native data arrays into
// the vtkDataArray interface.
//
// .SECTION Description
// Map native simulation code data arrays into the vtkDataArray interface.
// This is based on the vtkCPExodusIIResultsArrayTemplate.h class in VTK.

#ifndef vtkCPMappedVectorArrayTemplate_h
#define vtkCPMappedVectorArrayTemplate_h

#include "vtkMappedDataArray.h"

#include "vtkObjectFactory.h" // for vtkStandardNewMacro

template <class Scalar>
class vtkCPMappedVectorArrayTemplate : public vtkMappedDataArray<Scalar>
{
public:
  vtkAbstractTemplateTypeMacro(vtkCPMappedVectorArrayTemplate<Scalar>, vtkMappedDataArray<Scalar>)
    vtkMappedDataArrayNewInstanceMacro(
      vtkCPMappedVectorArrayTemplate<Scalar>) static vtkCPMappedVectorArrayTemplate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef typename Superclass::ValueType ValueType;

  // Description:
  // Set the raw scalar arrays for the coordinate set. This class takes
  // ownership of the arrays and deletes them with delete[].
  void SetVectorArray(Scalar* array, vtkIdType numPoints);

  // Reimplemented virtuals -- see superclasses for descriptions:
  void Initialize() override;
  void GetTuples(vtkIdList* ptIds, vtkAbstractArray* output) override;
  void GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray* output) override;
  void Squeeze() override;
  vtkArrayIterator* NewIterator() override;
  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;
  vtkVariant GetVariantValue(vtkIdType idx) override;
  void ClearLookup() override;
  double* GetTuple(vtkIdType i) override;
  void GetTuple(vtkIdType i, double* tuple) override;
  vtkIdType LookupTypedValue(Scalar value) override;
  void LookupTypedValue(Scalar value, vtkIdList* idse) override;
  ValueType GetValue(vtkIdType idx) const override;
  Scalar& GetValueReference(vtkIdType idx) override;
  void GetTypedTuple(vtkIdType idx, Scalar* t) const override;

  // Description:
  // This container is read only -- this method does nothing but print a
  // warning.
  int Allocate(vtkIdType sz, vtkIdType ext) override;
  int Resize(vtkIdType numTuples) override;
  void SetNumberOfTuples(vtkIdType number) override;
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;
  void SetTuple(vtkIdType i, const float* source) override;
  void SetTuple(vtkIdType i, const double* source) override;
  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source) override;
  void InsertTuple(vtkIdType i, const float* source) override;
  void InsertTuple(vtkIdType i, const double* source) override;
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override;
  void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(vtkIdType j, vtkAbstractArray* source) override;
  vtkIdType InsertNextTuple(const float* source) override;
  vtkIdType InsertNextTuple(const double* source) override;
  void DeepCopy(vtkAbstractArray* aa) override;
  void DeepCopy(vtkDataArray* da) override;
  void InterpolateTuple(
    vtkIdType i, vtkIdList* ptIndices, vtkAbstractArray* source, double* weights) override;
  void InterpolateTuple(vtkIdType i, vtkIdType id1, vtkAbstractArray* source1, vtkIdType id2,
    vtkAbstractArray* source2, double t) override;
  void SetVariantValue(vtkIdType idx, vtkVariant value) override;
  void InsertVariantValue(vtkIdType idx, vtkVariant value) override;
  void RemoveTuple(vtkIdType id) override;
  void RemoveFirstTuple() override;
  void RemoveLastTuple() override;
  void SetTypedTuple(vtkIdType i, const Scalar* t) override;
  void InsertTypedTuple(vtkIdType i, const Scalar* t) override;
  vtkIdType InsertNextTypedTuple(const Scalar* t) override;
  void SetValue(vtkIdType idx, Scalar value) override;
  vtkIdType InsertNextValue(Scalar v) override;
  void InsertValue(vtkIdType idx, Scalar v) override;

protected:
  vtkCPMappedVectorArrayTemplate();
  ~vtkCPMappedVectorArrayTemplate();

  Scalar* Array;

private:
  vtkCPMappedVectorArrayTemplate(const vtkCPMappedVectorArrayTemplate&) = delete;
  void operator=(const vtkCPMappedVectorArrayTemplate&) = delete;

  vtkIdType Lookup(const Scalar& val, vtkIdType startIndex);
  double TempDoubleArray[3];
};

#include "vtkCPMappedVectorArrayTemplate.txx"

#endif // vtkCPMappedVectorArrayTemplate_h
