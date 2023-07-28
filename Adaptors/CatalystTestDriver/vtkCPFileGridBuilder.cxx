// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPFileGridBuilder.h"

#include "vtkCPFieldBuilder.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"

vtkCxxSetObjectMacro(vtkCPFileGridBuilder, Grid, vtkDataObject);

//----------------------------------------------------------------------------
vtkCPFileGridBuilder::vtkCPFileGridBuilder()
{
  this->FileName = nullptr;
  this->KeepPointData = 1;
  this->KeepCellData = 1;
  this->Grid = nullptr;
}

//----------------------------------------------------------------------------
vtkCPFileGridBuilder::~vtkCPFileGridBuilder()
{
  this->SetFileName(nullptr);
  this->SetGrid(nullptr);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPFileGridBuilder::GetGrid()
{
  return this->Grid;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkCPFileGridBuilder::GetGrid(unsigned long timeStep, double time, int& builtNewGrid)
{
  builtNewGrid = 0;
  if (this->FileName == nullptr)
  {
    vtkWarningMacro("FileName is not set.");
    return nullptr;
  }

  vtkCPBaseFieldBuilder* fieldBuilder = this->GetFieldBuilder();
  if (fieldBuilder == nullptr && this->KeepPointData == 0 && this->KeepCellData == 0)
  {
    vtkWarningMacro("Need field data.");
    return nullptr;
  }

  if (!this->KeepPointData)
  {
    if (this->Grid->IsA("vtkCompositeDataSet"))
    {
      vtkCompositeDataIterator* iter = vtkCompositeDataSet::SafeDownCast(this->Grid)->NewIterator();
      iter->SkipEmptyNodesOn();
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet::SafeDownCast(iter->GetCurrentDataObject())->GetPointData()->Initialize();
      }
      iter->Delete();
    }
    else
    {
      vtkDataSet::SafeDownCast(this->Grid)->GetPointData()->Initialize();
    }
  }

  if (!this->KeepCellData)
  {
    if (this->Grid->IsA("vtkCompositeDataSet"))
    {
      vtkCompositeDataIterator* iter = vtkCompositeDataSet::SafeDownCast(this->Grid)->NewIterator();
      iter->SkipEmptyNodesOn();
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkDataSet::SafeDownCast(iter->GetCurrentDataObject())->GetCellData()->Initialize();
      }
      iter->Delete();
    }
    else
    {
      vtkDataSet::SafeDownCast(this->Grid)->GetCellData()->Initialize();
    }
  }

  if (fieldBuilder)
  {
    if (this->Grid->IsA("vtkCompositeDataSet"))
    {
      vtkCompositeDataIterator* iter = vtkCompositeDataSet::SafeDownCast(this->Grid)->NewIterator();
      iter->SkipEmptyNodesOn();
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        fieldBuilder->BuildField(
          timeStep, time, vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()));
      }
      iter->Delete();
    }
    else
    {
      fieldBuilder->BuildField(timeStep, time, vtkDataSet::SafeDownCast(this->Grid));
    }
  }
  builtNewGrid = 1;
  return this->Grid;
}
//----------------------------------------------------------------------------
void vtkCPFileGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "KeepPointData: " << this->KeepPointData << endl;
  os << indent << "KeepCellData: " << this->KeepCellData << endl;
  os << indent << "Grid: " << this->Grid << endl;
}
