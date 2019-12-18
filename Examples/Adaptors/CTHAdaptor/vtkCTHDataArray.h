#ifndef vtkCTHDataArray_h
#define vtkCTHDataArray_h

// #include "vtksnlIOWin32Header.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"

class VTK_EXPORT vtkCTHDataArray : public vtkDataArray
{
public:
  static vtkCTHDataArray* New();
  vtkTypeMacro(vtkCTHDataArray, vtkDataArray);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Prepares for new data
  void Initialize() override;

  // Description:
  // Get the data type
  int GetDataType() override { return VTK_DOUBLE; }

  int GetDataTypeSize() override { return static_cast<int>(sizeof(double)); }

  // Description:
  // Return the size, in bytes, of the lowest-level element of an
  // array.  For vtkDataArray and subclasses this is the size of the
  // data type.
  virtual int GetElementComponentSize() override { return this->GetDataTypeSize(); }

  // Description:
  // Set the dimensions the data will be contained within
  void SetDimensions(int x, int y, int z);
  void SetDimensions(int* x) { this->SetDimensions(x[0], x[1], x[2]); }
  const int* GetDimensions() { return this->Dimensions; }

  // Description:
  // Sets the extent over which the data is valid.
  // This is because with CTH cells can extend one past the boundary,
  // so we must skip over the data associated with those and consequently
  // act externally (as a vtkDataArray) as though our dimensions are one less.
  void SetExtents(int x0, int x1, int y0, int y1, int z0, int z1);
  void SetExtents(int* lo, int* hi) { this->SetExtents(lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]); }
  void SetExtent(int* x) { this->SetExtents(x[0], x[1], x[2], x[3], x[4], x[5]); }
  int* GetExtents() { return this->Extents; }
  void UnsetExtents();

  // Description:
  // Set the data pointers from the CTH code
  void SetDataPointer(int comp, int k, int j, double* istrip);

  // Description:
  // Copy the tuple value into a user-provided array.
  void GetTuple(vtkIdType i, double* tuple) override;
  double* GetTuple(vtkIdType i) override;

  // Description:
  // Returns an ArrayIterator over doubles, this will end up with a deep copy
  vtkArrayIterator* NewIterator() override;

  vtkIdType LookupValue(vtkVariant value) override;
  void LookupValue(vtkVariant value, vtkIdList* ids) override;
  void SetVariantValue(vtkIdType vtkNotUsed(index), vtkVariant vtkNotUsed(value)) override
  { /* TODO */}

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  double* GetPointer(vtkIdType id);
  void* GetVoidPointer(vtkIdType id) override { return this->GetPointer(id); }
  void ExportToVoidPointer(void* out_ptr) override;

  int Allocate(vtkIdType sz, vtkIdType ext = 1000) override
  {
    BuildFallback();
    return Fallback->Allocate(sz, ext);
  }

  void SetNumberOfComponents(int number) override
  {
    this->Superclass::SetNumberOfComponents(number);
    if (this->Fallback)
    {
      this->Fallback->SetNumberOfComponents(this->GetNumberOfComponents());
    }
  }

  void SetNumberOfTuples(vtkIdType number) override
  {
    this->BuildFallback();
    this->Fallback->SetNumberOfTuples(number);
    this->Size = this->Fallback->GetSize();
    this->MaxId = this->Fallback->GetMaxId();
  }

  // Description:
  // A number of abstract functions from the super class that must not be called
  void SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* aa) override
  {
    this->BuildFallback();
    this->Fallback->SetTuple(i, j, aa);
  }
  void SetTuple(vtkIdType i, const float* f) override
  {
    this->BuildFallback();
    this->Fallback->SetTuple(i, f);
  }
  void SetTuple(vtkIdType i, const double* d) override
  {
    this->BuildFallback();
    this->Fallback->SetTuple(i, d);
  }

  void InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* aa) override
  {
    this->BuildFallback();
    this->Fallback->InsertTuple(i, j, aa);
  }
  void InsertTuple(vtkIdType i, const float* f) override
  {
    this->BuildFallback();
    this->Fallback->InsertTuple(i, f);
  }
  void InsertTuple(vtkIdType i, const double* d) override
  {
    this->BuildFallback();
    this->Fallback->InsertTuple(i, d);
  }
  void InsertTuples(vtkIdList* dstIds, vtkIdList* srcIds, vtkAbstractArray* source) override
  {
    this->BuildFallback();
    this->Fallback->InsertTuples(dstIds, srcIds, source);
  }
  virtual void InsertTuples(
    vtkIdType dstStart, vtkIdType n, vtkIdType srcStart, vtkAbstractArray* source) override
  {
    this->BuildFallback();
    this->Fallback->InsertTuples(dstStart, n, srcStart, source);
  }

  vtkIdType InsertNextTuple(vtkIdType i, vtkAbstractArray* aa) override
  {
    this->BuildFallback();
    vtkIdType ret = this->Fallback->InsertNextTuple(i, aa);
    this->Size = this->Fallback->GetSize();
    this->MaxId = this->Fallback->GetMaxId();
    return ret;
  }
  vtkIdType InsertNextTuple(const float* f) override
  {
    this->BuildFallback();
    vtkIdType ret = this->Fallback->InsertNextTuple(f);
    this->Size = this->Fallback->GetSize();
    this->MaxId = this->Fallback->GetMaxId();
    return ret;
  }
  vtkIdType InsertNextTuple(const double* d) override
  {
    this->BuildFallback();
    vtkIdType ret = this->Fallback->InsertNextTuple(d);
    this->Size = this->Fallback->GetSize();
    this->MaxId = this->Fallback->GetMaxId();
    return ret;
  }

  void InsertVariantValue(vtkIdType idx, vtkVariant value) override
  {
    this->BuildFallback();
    return this->Fallback->InsertVariantValue(idx, value);
  }

  void RemoveTuple(vtkIdType id) override
  {
    this->BuildFallback();
    this->Fallback->RemoveTuple(id);
  }
  void RemoveFirstTuple() override
  {
    this->BuildFallback();
    this->Fallback->RemoveFirstTuple();
  }
  void RemoveLastTuple() override
  {
    this->BuildFallback();
    this->Fallback->RemoveLastTuple();
  }

  void* WriteVoidPointer(vtkIdType i, vtkIdType j) override
  {
    this->BuildFallback();
    return this->Fallback->WriteVoidPointer(i, j);
  }

  virtual void DeepCopy(vtkAbstractArray* aa) override
  {
    this->BuildFallback();
    return this->Fallback->DeepCopy(aa);
  }

  virtual void DeepCopy(vtkDataArray* da) override { return this->DeepCopy((vtkAbstractArray*)da); }

  void SetVoidArray(void* p, vtkIdType id, int i, int j) override
  {
    this->BuildFallback();
    return this->Fallback->SetVoidArray(p, id, i, j);
  }
  void SetVoidArray(void* p, vtkIdType id, int i) override
  {
    this->BuildFallback();
    return this->Fallback->SetVoidArray(p, id, i);
  }
  void SetArrayFreeFunction(void (*)(void*)) override {}

  // Description:
  // Since we don't allocate, this does nothing
  // unless we're already falling back
  void Squeeze() override
  {
    if (this->Fallback)
    {
      this->Size = this->Fallback->GetSize();
      this->MaxId = this->Fallback->GetMaxId();
      this->Fallback->Squeeze();
    }
  }

  // Description:
  int Resize(vtkIdType numTuples) override
  {
    this->BuildFallback();
    return this->Fallback->Resize(numTuples);
  }

  void DataChanged() override
  {
    this->BuildFallback();
    return this->Fallback->DataChanged();
  }

  void ClearLookup() override
  {
    this->BuildFallback();
    return this->Fallback->ClearLookup();
  }

protected:
  vtkCTHDataArray();
  ~vtkCTHDataArray();

  int Dimensions[3];

  bool ExtentsSet;
  int Extents[6];
  int Dx;
  int Dy;
  int Dz;

  vtkMTimeType PointerTime;

  double*** Data;
  double* CopiedData;
  int CopiedSize;
  double* Tuple;
  int TupleSize;

  void BuildFallback();
  // A writable version of this array, delegated.
  vtkDoubleArray* Fallback;

private:
  vtkCTHDataArray(const vtkCTHDataArray&) = delete;
  void operator=(const vtkCTHDataArray&) = delete;
};

#endif /* vtkCTHDataArray_h */
