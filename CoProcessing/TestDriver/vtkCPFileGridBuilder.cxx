/*=========================================================================

  Program:   ParaView
  Module:    vtkCPFileGridBuilder.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPFileGridBuilder.h"

#include "vtkCPFieldBuilder.h"
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
  this->FileName = 0;
  this->KeepPointData = 1;
  this->KeepCellData = 1;
  this->Grid = 0;
}

//----------------------------------------------------------------------------
vtkCPFileGridBuilder::~vtkCPFileGridBuilder()
{
  this->SetFileName(0);
  this->SetGrid(0);
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
  if (this->FileName == 0)
  {
    vtkWarningMacro("FileName is not set.");
    return 0;
  }

  vtkCPBaseFieldBuilder* fieldBuilder = this->GetFieldBuilder();
  if (fieldBuilder == 0 && this->KeepPointData == 0 && this->KeepCellData == 0)
  {
    vtkWarningMacro("Need field data.");
    return 0;
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
