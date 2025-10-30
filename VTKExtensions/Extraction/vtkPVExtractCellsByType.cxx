// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVExtractCellsByType.h"

#include <vtkCellType.h>
#include <vtkCellTypeUtilities.h>
#include <vtkCommand.h>
#include <vtkDataArraySelection.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <string>

vtkStandardNewMacro(vtkPVExtractCellsByType);

// ----------------------------------------------------------------------------
vtkPVExtractCellsByType::vtkPVExtractCellsByType()
{
  // Add observer for selection update
  this->CellTypeSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVExtractCellsByType::UpdateFromSelection);
}

//------------------------------------------------------------------------------
void vtkPVExtractCellsByType::UpdateFromSelection()
{
  this->RemoveAllCellTypes();

  for (int idx = 0; idx < this->CellTypeSelection->GetNumberOfArrays(); idx++)
  {
    if (this->CellTypeSelection->GetArraySetting(idx))
    {
      auto typeName = this->CellTypeSelection->GetArrayName(idx);
      this->AddCellType(vtkCellTypeUtilities::GetTypeIdFromName(typeName));
    }
  }
}
