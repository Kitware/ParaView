// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPIterator.h"

#include "vtkDSPMultiBlockIterator.h"
#include "vtkDSPTableIterator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDSPIterator> vtkDSPIterator::GetInstance(vtkDataObject* object)
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(object);
  vtkTable* table = vtkTable::SafeDownCast(object);

  if (mb)
  {
    auto instance = vtkSmartPointer<vtkDSPMultiBlockIterator>::New(mb);
    return instance;
  }
  else if (table)
  {
    auto instance = vtkSmartPointer<vtkDSPTableIterator>::New(table);
    return instance;
  }
  else
  {
    vtkWarningWithObjectMacro(nullptr,
      "Can't create iterator instance: "
      "input should be a vtkMultiBlockDataSet or a vtkTable.");
    return nullptr;
  }
}
