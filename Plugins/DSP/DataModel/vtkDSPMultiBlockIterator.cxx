// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDSPMultiBlockIterator.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
struct vtkDSPMultiBlockIterator::vtkInternals
{
  vtkSmartPointer<vtkCompositeDataIterator> Iterator;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkDSPMultiBlockIterator);

//-----------------------------------------------------------------------------
vtkDSPMultiBlockIterator* vtkDSPMultiBlockIterator::New(vtkMultiBlockDataSet* mb)
{
  auto result = vtkDSPMultiBlockIterator::New();
  result->Internals->Iterator.TakeReference(mb->NewIterator());
  result->Internals->Iterator->SkipEmptyNodesOn();
  return result;
}

//-----------------------------------------------------------------------------
vtkDSPMultiBlockIterator::vtkDSPMultiBlockIterator()
  : Internals(new vtkInternals)
{
}

//-----------------------------------------------------------------------------
void vtkDSPMultiBlockIterator::GoToFirstItem()
{
  this->Internals->Iterator->GoToFirstItem();
}

//-----------------------------------------------------------------------------
void vtkDSPMultiBlockIterator::GoToNextItem()
{
  this->Internals->Iterator->GoToNextItem();
}

//-----------------------------------------------------------------------------
bool vtkDSPMultiBlockIterator::IsDoneWithTraversal()
{
  return this->Internals->Iterator->IsDoneWithTraversal();
}

//-----------------------------------------------------------------------------
vtkTable* vtkDSPMultiBlockIterator::GetCurrentTable()
{
  vtkDataObject* object = this->Internals->Iterator->GetCurrentDataObject();
  vtkTable* table = vtkTable::SafeDownCast(object);

  if (object && !table)
  {
    vtkErrorMacro("Current block (flat index = " << this->Internals->Iterator->GetCurrentFlatIndex()
                                                 << ") is not a vtkTable!");
  }

  return table;
}
