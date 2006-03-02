/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkKWWindowLevelPresetSelector.h"

#include "vtkObjectFactory.h"
#include "vtkKWMultiColumnList.h"
#include "vtkKWMultiColumnListWithScrollbars.h"

const char *vtkKWWindowLevelPresetSelector::WindowColumnName = "Window";
const char *vtkKWWindowLevelPresetSelector::LevelColumnName  = "Level";
const char *vtkKWWindowLevelPresetSelector::ModalityColumnName  = "Modality";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindowLevelPresetSelector);
vtkCxxRevisionMacro(vtkKWWindowLevelPresetSelector, "1.15");

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetPresetWindow(
  int id, double val)
{
  return this->SetPresetUserSlotAsDouble(id, "Window", val);
}

//----------------------------------------------------------------------------
double vtkKWWindowLevelPresetSelector::GetPresetWindow(int id)
{
  return this->GetPresetUserSlotAsDouble(id, "Window");
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetPresetLevel(
  int id, double val)
{
  return this->SetPresetUserSlotAsDouble(id, "Level", val);
}

//----------------------------------------------------------------------------
double vtkKWWindowLevelPresetSelector::GetPresetLevel(int id)
{
  return this->GetPresetUserSlotAsDouble(id, "Level");
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetPresetModality(
  int id, const char *val)
{
  return this->SetPresetUserSlotAsString(id, "Modality", val);
}

//----------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::GetPresetModality(int id)
{
  return this->GetPresetUserSlotAsString(id, "Modality");
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::CreateColumns()
{
  this->Superclass::CreateColumns();

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  int col;

  // Modality

  col = list->InsertColumn(this->GetCommentColumnIndex(), 
                           vtkKWWindowLevelPresetSelector::ModalityColumnName);
  list->SetColumnName(col, vtkKWWindowLevelPresetSelector::ModalityColumnName);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 0);
  list->ColumnVisibilityOff(col);

  // Window

  col = list->InsertColumn(col + 1, "W");
  list->SetColumnName(col, vtkKWWindowLevelPresetSelector::WindowColumnName);
  list->SetColumnWidth(col, 6);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 1);
  list->SetColumnSortModeToReal(col);

  // Level

  col = list->InsertColumn(col + 1, "L");
  list->SetColumnName(col, vtkKWWindowLevelPresetSelector::LevelColumnName);
  list->SetColumnWidth(col, 6);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 1);
  list->SetColumnSortModeToReal(col);
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetWindowColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWWindowLevelPresetSelector::WindowColumnName) : -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetLevelColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWWindowLevelPresetSelector::LevelColumnName) : -1;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetModalityColumnIndex()
{
  return this->PresetList ? 
    this->PresetList->GetWidget()->GetColumnIndexWithName(
      vtkKWWindowLevelPresetSelector::ModalityColumnName) : -1;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::SetModalityColumnVisibility(int arg)
{
  if (this->PresetList)
    {
    this->PresetList->GetWidget()->SetColumnVisibility(
      this->GetModalityColumnIndex(), arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::GetModalityColumnVisibility()
{
  if (this->PresetList)
    {
    return this->PresetList->GetWidget()->GetColumnVisibility(
      this->GetModalityColumnIndex());
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::UpdatePresetRow(int id)
{
  if (!this->Superclass::UpdatePresetRow(id))
    {
    return 0;
    }

  int row = this->GetPresetRow(id);
  if (row < 0)
    {
    return 0;
    }

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  list->SetCellText(
    row, this->GetModalityColumnIndex(), this->GetPresetModality(id));
  
  list->SetCellTextAsDouble(
    row, this->GetWindowColumnIndex(), this->GetPresetWindow(id));

  list->SetCellTextAsDouble(
    row, this->GetLevelColumnIndex(), this->GetPresetLevel(id));
  
  return 1;
}

//---------------------------------------------------------------------------
const char* vtkKWWindowLevelPresetSelector::PresetCellEditEndCallback(
  int row, int col, const char *text)
{
  static char buffer[256];

  int id = this->GetPresetAtRowId(row);
  if (this->HasPreset(id))
    {
    if (col == this->GetWindowColumnIndex() || 
        col == this->GetLevelColumnIndex())
      {
      double val = atof(text);
      sprintf(buffer, "%g", val);
      return buffer;
      }
    }
  return this->Superclass::PresetCellEditEndCallback(row, col, text);
}

//---------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PresetCellUpdatedCallback(
  int row, int col, const char *text)
{
  int id = this->GetPresetAtRowId(row);
  if (this->HasPreset(id))
    {
    if (col == this->GetWindowColumnIndex() || 
        col == this->GetLevelColumnIndex())
      {
        double val = atof(text);
        if (col == this->GetWindowColumnIndex())
          {
          this->SetPresetWindow(id, val);
          }
        else
          {
          this->SetPresetLevel(id, val);
          }
        if (this->ApplyPresetOnSelection)
          {
          this->InvokePresetApplyCommand(id);
          }
        this->InvokePresetHasChangedCommand(id);
        return;
      }
    }

  this->Superclass::PresetCellUpdatedCallback(row, col, text);
}

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::HasPresetWithGroupWithWindowLevel(
  const char *group, double window, double level)
{
  int i, nb_presets = this->GetNumberOfPresetsWithGroup(group);
  for (i = 0; i < nb_presets; i++)
    {
    int id = this->GetNthPresetWithGroupId(i, group);
    if (this->GetPresetWindow(id) == window && 
        this->GetPresetLevel(id) == level)
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
