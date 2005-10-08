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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWWindowLevelPresetSelector);
vtkCxxRevisionMacro(vtkKWWindowLevelPresetSelector, "1.11");

//----------------------------------------------------------------------------
int vtkKWWindowLevelPresetSelector::SetPresetWindow(
  int id, double val)
{
  int res = this->SetPresetUserSlotAsDouble(id, "Window", val);
  if (res)
    {
    this->UpdatePresetRow(id);
    }
  return res;
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
  int res = this->SetPresetUserSlotAsDouble(id, "Level", val);
  if (res)
    {
    this->UpdatePresetRow(id);
    }
  return res;
}

//----------------------------------------------------------------------------
double vtkKWWindowLevelPresetSelector::GetPresetLevel(int id)
{
  return this->GetPresetUserSlotAsDouble(id, "Level");
}

//----------------------------------------------------------------------------
void vtkKWWindowLevelPresetSelector::CreateColumns()
{
  this->Superclass::CreateColumns();

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

  int col;

  col = list->InsertColumn(this->GetCommentColumnIndex(), 
                           vtkKWWindowLevelPresetSelector::WindowColumnName);
  list->SetColumnName(col, vtkKWWindowLevelPresetSelector::WindowColumnName);
  list->SetColumnWidth(col, 7);
  list->SetColumnResizable(col, 1);
  list->SetColumnStretchable(col, 0);
  list->SetColumnEditable(col, 1);
  list->SetColumnSortModeToReal(col);

  col = list->InsertColumn(col + 1, 
                           vtkKWWindowLevelPresetSelector::LevelColumnName);
  list->SetColumnName(col, vtkKWWindowLevelPresetSelector::LevelColumnName);
  list->SetColumnWidth(col, 7);
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
int vtkKWWindowLevelPresetSelector::UpdatePresetRow(int id)
{
  if (!this->Superclass::UpdatePresetRow(id))
    {
    return 0;
    }

  int row = this->GetPresetRow(id);

  vtkKWMultiColumnList *list = this->PresetList->GetWidget();

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
          this->InvokeApplyPresetCommand(id);
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
