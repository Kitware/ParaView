// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkArrayDispatchDSPArrayList_h
#define vtkArrayDispatchDSPArrayList_h

#include "vtkMultiDimensionalArray.h"
#include "vtkTypeList.h"

namespace vtkArrayDispatch
{
VTK_ABI_NAMESPACE_BEGIN

typedef vtkTypeList::Unique<vtkTypeList::Create<vtkMultiDimensionalArray<char>,
  vtkMultiDimensionalArray<double>, vtkMultiDimensionalArray<float>, vtkMultiDimensionalArray<int>,
  vtkMultiDimensionalArray<long>, vtkMultiDimensionalArray<long long>,
  vtkMultiDimensionalArray<short>, vtkMultiDimensionalArray<signed char>,
  vtkMultiDimensionalArray<unsigned char>, vtkMultiDimensionalArray<unsigned int>,
  vtkMultiDimensionalArray<unsigned long>, vtkMultiDimensionalArray<unsigned long long>,
  vtkMultiDimensionalArray<unsigned short>, vtkMultiDimensionalArray<vtkIdType>>>::Result
  MultiDimensionalArrays;

VTK_ABI_NAMESPACE_END

} // end namespace vtkArrayDispatch
#endif // vtkArrayDispatchDSPArrayList_h
