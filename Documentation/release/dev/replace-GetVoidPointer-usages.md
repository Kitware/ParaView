## vtkAbstractArray: Replace most of GetVoidPointer instances

`vtkAbstractArray::GetVoidPointer()` is a function that returns a raw pointer to the underlying data of a VTK array.
This function was added in VTK's early days when it only supported `vtkAOSDataArrayTemplate` arrays. Since then,
VTK has added support for multiple other array types, either explicit (e.g., `vtkSOADataArrayTemplate`,
`vtkScaledSOADataArrayTemplate`, etc.) or implicit (e.g., `vtkAffineArray`, `vtkConstantArray`, etc.). These array types
don't store their data in a contiguous block of memory, so using `GetVoidPointer()` leads to duplication of data in
an internal `vtkAOSDataArrayTemplate` array, which leads to memory usage increase and unnecessary copying overhead.

In attempt to modernize ParaView and reduce unnecessary data duplication and copying overhead, ~94% (73/78) of the
`GetVoidPointer()` usages have been removed using one of the following techniques:

1. If array is `vtkAOSDataArrayTemplate`, use `GetPointer()` instead of `GetVoidPointer()`.
2. If array is `vtkDataArray` and all tuples need to be filled, use `Fill` instead of `memset()` with
   `GetVoidPointer()`.
3. If array is `vtkAbstractArray` and some tuples need to be copied, use `InsertTuples*()` methods instead of
   `GetVoidPointer()` and `memcpy()`.
4. If array is `vtkDataArray` and access to the tuples/values is needed, use `vtkArrayDispatch` and
   `vtk::DataArray(Value/Tuple)Range()`, instead of `vtkTemplateMacro` with `GetVoidPointer()`.

Moreover, the remaining 5 `GetVoidPointer()` usages have been marked as _safe_ using the `clang-tidy` lint
`(bugprone-unsafe-functions)` when one of the following conditions is met:

1. If array is `vtkDataArray`, but using `HasStandardMemoryLayout`, it has been verified that it actually is
   `vtkAOSDataArrayTemplate`, then `GetVoidPointer()` is _safe_ to use. This usually happens when the array was created
   using `vtkDataArray::CreateDataArray()` or `vtkAbstractArray::CreateArray()` by passing a standard VTK data type.
2. If array is `vtkDataArray`, the data type is NOT known, but raw pointer to data is absolutely needed, use
   `auto aos = array->ToAOSDataArray()`, and then use `aos->GetVoidPointer()`, which is _safe_, being aware that this
   may lead to data duplication if the array is not `vtkAOSDataArrayTemplate`.

Finally, `vtkAbstractArray::GetVoidPointer()` and `vtkDataArray::GetVoidPointer()` have been marked as an _unsafe_
function using `clang-tidy`, and its usage will be warned as such, unless explicitly marked as _safe_ using the
`(bugprone-unsafe-functions)` lint, upon guaranteeing that it is indeed safe to use, if and only if, it satisfies one of
the conditions mentioned above.
