/*=========================================================================

  Module:    vtkKWMultiColumnList.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWMultiColumnList.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMultiColumnList);
vtkCxxRevisionMacro(vtkKWMultiColumnList, "1.9");

//----------------------------------------------------------------------------
vtkKWMultiColumnList::vtkKWMultiColumnList()
{
  this->SelectionChangedCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWMultiColumnList::~vtkKWMultiColumnList()
{
  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::Create(vtkKWApplication *app)
{
  this->Script("package require tablelist");

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "tablelist::tablelist"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetBorderWidth(0);
  this->SetBackgroundColor(0.98, 0.98, 0.98);

  this->SetHeight(15);
  this->SetShowSeparators(0);

  this->SetConfigurationOption(
    "-stretch", "all");
  this->SetConfigurationOption(
    "-labelcommand", "tablelist::sortByColumn");
  this->SetConfigurationOption(
    "-stripebackground", "#e0e8f0");
  this->SetConfigurationOptionAsInt(
    "-highlightthickness", 0);
  this->SetConfigurationOption(
    "-activestyle", "frame");

  this->Script("bind %s <<TablelistSelect>> {+ %s SelectionChangedCallback}",
               this->GetWidgetName(), this->GetTclName());
  
  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddColumn(const char *title, 
                                     int width, 
                                     int align)
{
  if (this->IsCreated() && title)
    {
    const char *alignment_opt;
    switch (align)
      {
      case vtkKWMultiColumnList::ColumnAlignmentLeft:
        alignment_opt = "left";
        break;
      case vtkKWMultiColumnList::ColumnAlignmentRight:
        alignment_opt = "right";
        break;
      case vtkKWMultiColumnList::ColumnAlignmentCenter:
        alignment_opt = "center";
        break;
      default:
        alignment_opt = "";
      }
    this->Script("%s insertcolumns end %d {%s} %s", 
                 this->GetWidgetName(), 
                 width, title, alignment_opt);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnWidth(int col_index, int width)
{
  if (this->IsCreated())
    {
    this->Script("%s columnconfigure %d -width %d", 
                 this->GetWidgetName(), col_index, width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnWidth(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -width", 
                                   this->GetWidgetName(),col_index);
    if (val && *val)
      {
      return atoi(val);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnMaximumWidth(int col_index, int width)
{
  if (this->IsCreated())
    {
    this->Script("%s columnconfigure %d -maxwidth %d", 
                 this->GetWidgetName(), col_index, width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnMaximumWidth(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -maxwidth", 
                                   this->GetWidgetName(),col_index);
    if (val && *val)
      {
      return atoi(val);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnResizable(int col_index, int flag)
{
  if (this->IsCreated())
    {
    this->Script("%s columnconfigure %d -resizable %d", 
                 this->GetWidgetName(), col_index, flag);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnResizable(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -resizable", 
                                   this->GetWidgetName(),col_index);
    if (val && *val)
      {
      return atoi(val) ? 1 : 0;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnEditable(int col_index, int flag)
{
  if (this->IsCreated())
    {
    this->Script("%s columnconfigure %d -editable %d", 
                 this->GetWidgetName(), col_index, flag);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnEditable(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -editable", 
                                   this->GetWidgetName(),col_index);
    if (val && *val)
      {
      return atoi(val) ? 1 : 0;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetWidth(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", 
                 this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetHeight(int height)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -height %d", 
                 this->GetWidgetName(), height);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetEditStartCommand(vtkObject* object, 
                                               const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("%s configure -editstartcommand {%s}", 
                 this->GetWidgetName(), command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetEditEndCommand(vtkObject* object, 
                                             const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("%s configure -editendcommand {%s}", 
                 this->GetWidgetName(), command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetLabelCommand(vtkObject* object, 
                                           const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("%s configure -labelcommand {%s}", 
                 this->GetWidgetName(), command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSortCommand(vtkObject* object, 
                                          const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("%s configure -sortcommand {%s}", 
                 this->GetWidgetName(), command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetMovableColumns(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -movablecolumns %d", 
                 this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetMovableColumns()
{
  return this->GetConfigurationOptionAsInt("-movablecolumns");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetResizableColumns(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -resizablecolumns %d", 
                 this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetResizableColumns()
{
  return this->GetConfigurationOptionAsInt("-resizablecolumns");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetShowSeparators(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -showseparators %d", 
                 this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetShowSeparators()
{
  return this->GetConfigurationOptionAsInt("-showseparators");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureColumnOptions(int col_index, 
                                                  const char *options)
{
  if (this->IsCreated() && options)
    {
    this->Script("%s columnconfigure %d %s", 
                 this->GetWidgetName(), col_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureRowOptions(int row_index, 
                                               const char *options)
{
  if (this->IsCreated() && options)
    {
    this->Script("%s rowconfigure %d %s", 
                 this->GetWidgetName(), row_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureCellOptions(int row_index, 
                                                int col_index, 
                                                const char *options)
{
  if (this->IsCreated() && options)
    {
    this->Script("%s cellconfigure %d,%d %s", 
                 this->GetWidgetName(), 
                 row_index, col_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellWindowCommand(int row_index, 
                                                int col_index, 
                                                vtkObject* object, 
                                                const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->Script("%s cellconfigure %d,%d -window {%s}", 
                 this->GetWidgetName(), 
                 row_index, col_index, command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertCellText(
  int row_index, int col_index, const char *text)
{
  if (this->IsCreated() && text)
    {
    while (row_index > this->GetNumberOfRows() - 1)
      {
      this->AddRow();
      }
    this->SetCellText(row_index, col_index, text);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellText(
  int row_index, int col_index, const char *text)
{
  if (this->IsCreated() && text)
    {
    this->Script("%s cellconfigure %d,%d -text {%s}", 
                 this->GetWidgetName(), row_index, col_index, text);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellText(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    return this->Script("%s cellcget %d,%d -text", 
                        this->GetWidgetName(), 
                        row_index, col_index);
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::FindCellText(
  const char *text, int *row_index, int *col_index)
{
  if (this->IsCreated() && 
      text && row_index && col_index)
    {
    int nb_cols = this->GetNumberOfColumns();
    int nb_rows = this->GetNumberOfRows();

    for (int j = 0; j < nb_rows; j++)
      {
      for (int i = 0; i < nb_cols; i++)
        {
        const char *cell_text = this->GetCellText(j, i);
        if (cell_text && !strcmp(cell_text, text))
          {
          *row_index = j;
          *col_index = i;
          return 1;
          }
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::FindCellTextInColumn(
  int col_index, const char *text, int *row_index)
{
  if (this->IsCreated() && 
      text && row_index)
    {
    int nb_rows = this->GetNumberOfRows();

    for (int j = 0; j < nb_rows; j++)
      {
      const char *cell_text = this->GetCellText(j, col_index);
      if (cell_text && !strcmp(cell_text, text))
        {
        *row_index = j;
        return 1;
        }
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::EditCellText(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s editcell %d,%d", 
                 this->GetWidgetName(), 
                 row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertRow(int row_index)
{
  if (this->IsCreated())
    {
    int nb_cols = this->GetNumberOfColumns();
    if (nb_cols > 0)
      {
      vtksys_stl::string item;
      for (int i = 0; i < nb_cols; i++)
        {
        item += "\"\" ";
        }
      this->Script("%s insert %d {%s}", 
                   this->GetWidgetName(), row_index, item.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddRow()
{
  this->InsertRow(this->GetNumberOfRows());
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ClearSelection()
{
  if (this->IsCreated())
    {
    this->Script("%s selection clear 0 end", 
                 this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectSingleRow(int row_index)
{
  if (this->IsCreated())
    {
    this->ClearSelection();
    this->Script("%s selection set %d %d", 
                 this->GetWidgetName(), row_index, row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetIndexOfFirstSelectedRow()
{
  if (this->IsCreated())
    {
    const char *sel = this->Script("lindex [%s curselection] 0", 
                                   this->GetWidgetName());
    if (sel && *sel)
      {
      return atoi(sel);
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectionChangedCallback()
{
  if (this->SelectionChangedCommand)
    {
    this->Script("eval %s", this->SelectionChangedCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ActivateRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s activate %d", 
                 this->GetWidgetName(), row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetNumberOfRows()
{
  if (this->IsCreated())
    {
    const char *val = 
      this->Script("%s size", this->GetWidgetName());
    if (val && *val)
      {
      return atoi(val);
      }
    return 0;
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetNumberOfColumns()
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncount", 
                                   this->GetWidgetName());
    if (val && *val)
      {
      return atoi(val);
      }
    return 0;
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeleteAllRows()
{
  if (this->IsCreated())
    {
    this->Script("%s delete 0 end", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SortByColumn(int col_index, 
                                        int increasing_order)
{
  if (this->IsCreated())
    {
    this->Script("%s sortbycolumn %d %s", 
                 this->GetWidgetName(), 
                 col_index, 
                 (increasing_order ? "-increasing" : "-decreasing"));
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
