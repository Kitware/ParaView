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
#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMultiColumnList);
vtkCxxRevisionMacro(vtkKWMultiColumnList, "1.1");

//----------------------------------------------------------------------------
vtkKWMultiColumnList::vtkKWMultiColumnList()
{
  this->TableList           = NULL;
  this->VerticalScrollBar   = NULL;
  this->HorizontalScrollBar = NULL;
  this->SelectionChangedCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWMultiColumnList::~vtkKWMultiColumnList()
{
  if (this->TableList)
    {
    this->TableList->Delete();
    this->TableList = NULL;
    }

  if (this->VerticalScrollBar)
    {
    this->VerticalScrollBar->Delete();
    this->VerticalScrollBar = NULL;
    }

  if (this->HorizontalScrollBar)
    {
    this->HorizontalScrollBar->Delete();
    this->HorizontalScrollBar = NULL;
    }

  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", "-bd 2 -relief sunken"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Create the tablelist

  this->Script("package require tablelist");

  if (!this->TableList)
    {
    this->TableList = vtkKWWidget::New();
    }

  vtkstd::string all_args(
    "-bd 0 "
    "-stretch all "
    "-labelcommand tablelist::sortByColumn "
    "-background gray98 "
    "-stripebackground #e0e8f0 "
    "-highlightthickness 0 "
    "-height 15 "
    "-activestyle frame "
    "-showseparators 0"
    );
  if (args)
    {
    all_args += args;
    }
  this->TableList->SetParent(this);
  this->TableList->Create(app, "tablelist::tablelist", all_args.c_str());

  this->Script("bind %s <<TablelistSelect>> {+ %s SelectionChangedCallback}",
               this->TableList->GetWidgetName(), this->GetTclName());
  
  // Create the scrollbars

  if (this->UseVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(app);
    }

  if (this->UseHorizontalScrollbar)
    {
    this->CreateHorizontalScrollbar(app);
    }

  // Pack

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::CreateVerticalScrollbar(vtkKWApplication *app)
{
  if (!this->VerticalScrollBar)
    {
    this->VerticalScrollBar = vtkKWWidget::New();
    }

  if (!this->VerticalScrollBar->IsCreated())
    {
    this->VerticalScrollBar->SetParent(this);
    this->VerticalScrollBar->Create(app, "scrollbar", "-orient vertical");
    if (this->TableList && this->TableList->IsCreated())
      {
      vtkstd::string command("-command {");
      command += this->TableList->GetWidgetName();
      command += " yview}";
      this->VerticalScrollBar->ConfigureOptions(command.c_str());
      command = "-yscrollcommand {";
      command += this->VerticalScrollBar->GetWidgetName();
      command += " set}";
      this->TableList->ConfigureOptions(command.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::CreateHorizontalScrollbar(vtkKWApplication *app)
{
  if (!this->HorizontalScrollBar)
    {
    this->HorizontalScrollBar = vtkKWWidget::New();
    }

  if (!this->HorizontalScrollBar->IsCreated())
    {
    this->HorizontalScrollBar->SetParent(this);
    this->HorizontalScrollBar->Create(app, "scrollbar", "-orient horizontal");
    if (this->TableList && this->TableList->IsCreated())
      {
      vtkstd::string command("-command {");
      command += this->TableList->GetWidgetName();
      command += " xview}";
      this->HorizontalScrollBar->ConfigureOptions(command.c_str());
      command = "-xscrollcommand {";
      command += this->HorizontalScrollBar->GetWidgetName();
      command += " set}";
      this->TableList->ConfigureOptions(command.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->UnpackChildren();

  ostrstream tk_cmd;

  if (this->TableList && this->TableList->IsCreated())
    {
    tk_cmd << "grid " << this->TableList->GetWidgetName() 
           << " -row 0 -column 0 -sticky news" << endl;
    }

  if (this->UseVerticalScrollbar && 
      this->VerticalScrollBar && this->VerticalScrollBar->IsCreated())
    {
    tk_cmd << "grid " << this->VerticalScrollBar->GetWidgetName() 
           << " -row 0 -column 1 -sticky ns" << endl;
    }

  if (this->UseHorizontalScrollbar && 
      this->HorizontalScrollBar && this->HorizontalScrollBar->IsCreated())
    {
    tk_cmd << "grid " << this->HorizontalScrollBar->GetWidgetName() 
           << " -row 1 -column 0 -sticky ew" << endl;
    }

  tk_cmd << "grid rowconfigure " << this->GetWidgetName() << " 0 -weight 1" 
         << endl;
  tk_cmd << "grid columnconfigure " << this->GetWidgetName() << " 0 -weight 1" 
         << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetUseVerticalScrollbar(int arg)
{
  if (this->UseVerticalScrollbar == arg)
    {
    return;
    }

  this->UseVerticalScrollbar = arg;
  if (this->UseVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(this->GetApplication());
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetUseHorizontalScrollbar(int arg)
{
  if (this->UseHorizontalScrollbar == arg)
    {
    return;
    }

  this->UseHorizontalScrollbar = arg;
  if (this->UseHorizontalScrollbar)
    {
    this->CreateHorizontalScrollbar(this->GetApplication());
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddColumn(const char *title, 
                                     int width, 
                                     int align)
{
  if (this->TableList && this->TableList->IsCreated() && title)
    {
    const char *alignment_opt;
    switch (align)
      {
      case vtkKWMultiColumnList::COLUMN_ALIGNMENT_LEFT:
        alignment_opt = "left";
        break;
      case vtkKWMultiColumnList::COLUMN_ALIGNMENT_RIGHT:
        alignment_opt = "right";
        break;
      case vtkKWMultiColumnList::COLUMN_ALIGNMENT_CENTER:
        alignment_opt = "center";
        break;
      default:
        alignment_opt = "";
      }
    this->Script("%s insertcolumns end %d {%s} %s", 
                 this->TableList->GetWidgetName(), 
                 width, title, alignment_opt);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnWidth(int col_index, int width)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s columnconfigure %d -width %d", 
                 this->TableList->GetWidgetName(), col_index, width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnWidth(int col_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -width", 
                                   this->TableList->GetWidgetName(),col_index);
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s columnconfigure %d -maxwidth %d", 
                 this->TableList->GetWidgetName(), col_index, width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnMaximumWidth(int col_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -maxwidth", 
                                   this->TableList->GetWidgetName(),col_index);
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s columnconfigure %d -resizable %d", 
                 this->TableList->GetWidgetName(), col_index, flag);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnResizable(int col_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -resizable", 
                                   this->TableList->GetWidgetName(),col_index);
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s columnconfigure %d -editable %d", 
                 this->TableList->GetWidgetName(), col_index, flag);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnEditable(int col_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = this->Script("%s columncget %d -editable", 
                                   this->TableList->GetWidgetName(),col_index);
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -width %d", 
                 this->TableList->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetWidth()
{
  if (this->TableList)
    {
    return this->TableList->GetConfigurationOptionAsInt("-width");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetHeight(int height)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -height %d", 
                 this->TableList->GetWidgetName(), height);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetHeight()
{
  if (this->TableList)
    {
    return this->TableList->GetConfigurationOptionAsInt("-height");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetEditStartCommand(vtkKWObject* object, 
                                               const char *method)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -editstartcommand {%s %s}", 
                 this->TableList->GetWidgetName(),
                 object->GetTclName(), method);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetEditEndCommand(vtkKWObject* object, 
                                             const char *method)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -editendcommand {%s %s}", 
                 this->TableList->GetWidgetName(),
                 object->GetTclName(), method);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetLabelCommand(vtkKWObject* object, 
                                             const char *method)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -labelcommand {%s %s}", 
                 this->TableList->GetWidgetName(),
                 object->GetTclName(), method);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSortCommand(vtkKWObject* object, 
                                             const char *method)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -sortcommand {%s %s}", 
                 this->TableList->GetWidgetName(),
                 object->GetTclName(), method);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetMovableColumns(int width)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -movablecolumns %d", 
                 this->TableList->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetMovableColumns()
{
  if (this->TableList)
    {
    return this->TableList->GetConfigurationOptionAsInt("-movablecolumns");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetResizableColumns(int width)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -resizablecolumns %d", 
                 this->TableList->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetResizableColumns()
{
  if (this->TableList)
    {
    return this->TableList->GetConfigurationOptionAsInt("-resizablecolumns");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetShowSeparators(int width)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s configure -showseparators %d", 
                 this->TableList->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetShowSeparators()
{
  if (this->TableList)
    {
    return this->TableList->GetConfigurationOptionAsInt("-showseparators");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureColumnOptions(int col_index, 
                                                  const char *options)
{
  if (this->TableList && this->TableList->IsCreated() && options)
    {
    this->Script("%s columnconfigure %d %s", 
                 this->TableList->GetWidgetName(), col_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureRowOptions(int row_index, 
                                               const char *options)
{
  if (this->TableList && this->TableList->IsCreated() && options)
    {
    this->Script("%s rowconfigure %d %s", 
                 this->TableList->GetWidgetName(), row_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ConfigureCellOptions(int row_index, 
                                                int col_index, 
                                                const char *options)
{
  if (this->TableList && this->TableList->IsCreated() && options)
    {
    this->Script("%s cellconfigure %d,%d %s", 
                 this->TableList->GetWidgetName(), 
                 row_index, col_index, options);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellWindowCommand(int row_index, 
                                                int col_index, 
                                                vtkKWObject* object, 
                                                const char *method)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s cellconfigure %d,%d -window {%s %s}", 
                 this->TableList->GetWidgetName(), 
                 row_index, col_index,
                 object->GetTclName(), method);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertCellText(
  int row_index, int col_index, const char *text)
{
  if (this->TableList && this->TableList->IsCreated() && text)
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
  if (this->TableList && this->TableList->IsCreated() && text)
    {
    this->Script("%s cellconfigure %d,%d -text {%s}", 
                 this->TableList->GetWidgetName(), row_index, col_index, text);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellText(int row_index, int col_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    return this->Script("%s cellcget %d,%d -text", 
                        this->TableList->GetWidgetName(), 
                        row_index, col_index);
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::FindCellText(
  const char *text, int *row_index, int *col_index)
{
  if (this->TableList && this->TableList->IsCreated() && 
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
  if (this->TableList && this->TableList->IsCreated() && 
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s editcell %d,%d", 
                 this->TableList->GetWidgetName(), 
                 row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertRow(int row_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    int nb_cols = this->GetNumberOfColumns();
    if (nb_cols > 0)
      {
      vtkstd::string item;
      for (int i = 0; i < nb_cols; i++)
        {
        item += "\"\" ";
        }
      this->Script("%s insert %d {%s}", 
                   this->TableList->GetWidgetName(), row_index, item.c_str());
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s selection clear 0 end", 
                 this->TableList->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectSingleRow(int row_index)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->ClearSelection();
    this->Script("%s selection set %d %d", 
                 this->TableList->GetWidgetName(), row_index, row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetIndexOfFirstSelectedRow()
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *sel = this->Script("lindex [%s curselection] 0", 
                                   this->TableList->GetWidgetName());
    if (sel && *sel)
      {
      return atoi(sel);
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionChangedCommand(
  vtkKWObject *object, const char *method)
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s activate %d", 
                 this->TableList->GetWidgetName(), row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetNumberOfRows()
{
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = 
      this->Script("%s size", this->TableList->GetWidgetName());
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
  if (this->TableList && this->TableList->IsCreated())
    {
    const char *val = this->Script("%s columncount", 
                                   this->TableList->GetWidgetName());
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
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s delete 0 end", this->TableList->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SortByColumn(int col_index, 
                                        int increasing_order)
{
  if (this->TableList && this->TableList->IsCreated())
    {
    this->Script("%s sortbycolumn %d %s", 
                 this->TableList->GetWidgetName(), 
                 col_index, 
                 (increasing_order ? "-increasing" : "-decreasing"));
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->TableList);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "UseVerticalScrollbar: " 
     << (this->UseVerticalScrollbar ? "On" : "Off") << endl;
  os << indent << "UseHorizontalScrollbar: " 
     << (this->UseHorizontalScrollbar ? "On" : "Off") << endl;
  os << indent << "TableList: ";
  if (this->TableList)
    {
    os << this->TableList << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "VerticalScrollBar: ";
  if (this->VerticalScrollBar)
    {
    os << this->VerticalScrollBar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "HorizontalScrollBar: ";
  if (this->HorizontalScrollBar)
    {
    os << this->HorizontalScrollBar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
}
