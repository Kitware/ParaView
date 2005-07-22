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
#include "vtkKWIcon.h"

#include <vtksys/stl/string>

#include "Utilities/Tablelist/vtkKWTablelist.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWMultiColumnList);
vtkCxxRevisionMacro(vtkKWMultiColumnList, "1.10");

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
  // Use Tablelist class:
  // http://www.nemethi.de/

  vtkKWTablelist::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "tablelist::tablelist"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetBackgroundColor(0.98, 0.98, 0.98);
  this->SetStripeBackgroundColor(0.878, 0.909, 0.941);
  this->SetWidth(0);
  this->SetHeight(15);
  this->SetColumnSeparatorsVisibility(1);
  this->SetSortArrowVisibility(0);
  this->SetHighlightThickness(0);
  this->SetSelectionModeToSingle();
  this->SetLabelCommand(NULL, "tablelist::sortByColumn");
  this->SetReliefToSunken();
  this->SetBorderWidth(2);

  this->SetConfigurationOption("-activestyle", "none");

  this->AddBinding("<<TablelistSelect>>", this, "SelectionChangedCallback");
  
  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetHeight(int height)
{
  this->SetConfigurationOptionAsInt("-height", height);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::AddColumn(const char *title)
{
  if (this->IsCreated() && title)
    {
    this->Script("%s insertcolumns end 0 {%s}", this->GetWidgetName(), title);
    return this->GetNumberOfColumns() - 1;
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetNumberOfColumns()
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s columncount", this->GetWidgetName());
    if (val && *val)
      {
      return atoi(val);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SeeColumn(int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s seecolumn %d", this->GetWidgetName(), col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeleteColumn(int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s deletecolumns %d %d", 
                 this->GetWidgetName(), col_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeleteAllColumns()
{
  if (this->IsCreated())
    {
    this->Script("%s deletecolumns 0 end", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetMovableColumns(int arg)
{
  this->SetConfigurationOptionAsInt("-movablecolumns", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetMovableColumns()
{
  return this->GetConfigurationOptionAsInt("-movablecolumns");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetResizableColumns(int arg)
{
  this->SetConfigurationOptionAsInt("-resizablecolumns", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetResizableColumns()
{
  return this->GetConfigurationOptionAsInt("-resizablecolumns");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnSeparatorsVisibility(int arg)
{
  this->SetConfigurationOptionAsInt("-showseparators", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnSeparatorsVisibility()
{
  return this->GetConfigurationOptionAsInt("-showseparators");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelsVisibility(int arg)
{
  this->SetConfigurationOptionAsInt("-showlabels", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnLabelsVisibility()
{
  return this->GetConfigurationOptionAsInt("-showlabels");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnLabelBackgroundColor(
  double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-labelbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnLabelBackgroundColor()
{
  static double rgb[3];
  this->GetColumnLabelBackgroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelBackgroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-labelbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnLabelForegroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-labelforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnLabelForegroundColor()
{
  static double rgb[3];
  this->GetColumnLabelForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-labelforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnTitle(int col_index, const char *title)
{
  this->SetColumnConfigurationOptionAsText(col_index, "-title", title);
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetColumnTitle(int col_index)
{
  return this->GetColumnConfigurationOptionAsText(col_index, "-title");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnWidth(int col_index, int width)
{
  this->SetColumnConfigurationOptionAsInt(col_index, "-width", width);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnWidth(int col_index)
{
  return this->GetColumnConfigurationOptionAsInt(col_index, "-width");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnMaximumWidth(int col_index, int width)
{
  this->SetColumnConfigurationOptionAsInt(col_index, "-maxwidth", width);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnMaximumWidth(int col_index)
{
  return this->GetColumnConfigurationOptionAsInt(col_index, "-maxwidth");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnStretchable(int col_index, int flag)
{
  if (!this->IsCreated())
    {
    return;
    }

  char buffer[10];
  vtksys_stl::string stretch(this->GetConfigurationOption("-stretch"));

  // If we are stretching all columns already, do not bother
  // Otherwise translate all into a list of all columns

  if (!strcmp("all", stretch.c_str()))
    {
    if (flag == 1)
      {
      return;
      }
    stretch = "";
    int nb_columns = this->GetNumberOfColumns();
    for (int i = 0; i < nb_columns; i++)
      {
      sprintf(buffer, " %d", i);
      stretch += buffer;
      }
    }

  // Is the column in the stretch list already. Remove or add it if needed

  int pos = atoi(
    this->Script("lsearch -exact {%s} %d", stretch.c_str(), col_index));

  if (flag == 1)
    {
    if (pos < 0)
      {
      sprintf(buffer, " %d", col_index);
      stretch += buffer;
      this->SetConfigurationOption("-stretch", stretch.c_str());
      }
    }
  else
    {
    if (pos >= 0)
      {
      stretch = this->Script("lreplace {%s} %d %d", stretch.c_str(), pos, pos);
      this->SetConfigurationOption("-stretch", stretch.c_str());
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnStretchable(int col_index)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  vtksys_stl::string stretch(this->GetConfigurationOption("-stretch"));

  // If we are stretching all columns already, do not bother

  if (!strcmp("all", stretch.c_str()))
    {
    return 1;
    }
  
  // Is the column in the stretch list

  int pos = atoi(
    this->Script("lsearch -exact {%s} %d", stretch.c_str(), col_index));
  return pos >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetStretchableColumns(int arg)
{
  this->SetConfigurationOption("-stretch", arg ? "all" : "");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnAlignment(int col_index, int align)
{
  if (!this->IsCreated())
    {
    return;
    }

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
      alignment_opt = "left";
      break;
    }
  this->SetColumnConfigurationOption(col_index, "-align", alignment_opt);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnAlignment(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = this->GetColumnConfigurationOption(col_index, "-align");
    if (val && *val)
      {
      if (!strcmp(val, "left"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentLeft;
        }
      if (!strcmp(val, "right"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentRight;
        }
      if (!strcmp(val, "center"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentCenter;
        }
      }
    }

  return vtkKWMultiColumnList::ColumnAlignmentUnknown;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelAlignment(int col_index, int align)
{
  if (!this->IsCreated())
    {
    return;
    }

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
      alignment_opt = "left";
      break;
    }
  this->SetColumnConfigurationOption(col_index, "-labelalign", alignment_opt);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnLabelAlignment(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = 
      this->GetColumnConfigurationOption(col_index, "-labelalign");
    if (val && *val)
      {
      if (!strcmp(val, "left"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentLeft;
        }
      if (!strcmp(val, "right"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentRight;
        }
      if (!strcmp(val, "center"))
        {
        return vtkKWMultiColumnList::ColumnAlignmentCenter;
        }
      }
    }

  return vtkKWMultiColumnList::ColumnAlignmentUnknown;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnResizable(int col_index, int flag)
{
  this->SetColumnConfigurationOptionAsInt(col_index, "-resizable", flag ?1:0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnResizable(int col_index)
{
  return this->GetColumnConfigurationOptionAsInt(col_index, "-resizable");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnEditable(int col_index, int flag)
{
  this->SetColumnConfigurationOptionAsInt(col_index, "-editable", flag ?1:0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnEditable(int col_index)
{
  return this->GetColumnConfigurationOptionAsInt(col_index, "-editable");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnVisibility(int col_index, int flag)
{
  this->SetColumnConfigurationOptionAsInt(col_index, "-hide", flag ?0:1);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnVisibility(int col_index)
{
  return this->GetColumnConfigurationOptionAsInt(col_index, "-hide") ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnBackgroundColor(
  int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetColumnConfigurationOption(col_index, "-bg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnBackgroundColor(int col_index)
{
  static double rgb[3];
  this->GetColumnBackgroundColor(col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnBackgroundColor(
  int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetColumnConfigurationOption(col_index, "-bg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnForegroundColor(
  int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetColumnConfigurationOption(col_index, "-fg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnForegroundColor(int col_index)
{
  static double rgb[3];
  this->GetColumnForegroundColor(col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnForegroundColor(
  int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetColumnConfigurationOption(col_index, "-fg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelImage(
  int col_index, const char *image_name)
{
  this->SetColumnConfigurationOption(col_index, "-labelimage", image_name);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelImageToPredefinedIcon(
  int col_index, int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetColumnLabelImageToIcon(col_index, icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelImageToIcon(
  int col_index, vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetColumnLabelImageToPixels(
      col_index, 
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnLabelImageToPixels(
  int col_index,
  const unsigned char* pixels, 
  int width, 
  int height,
  int pixel_size,
  unsigned long buffer_length)
{
  static int col_label_img_counter = 0;

  if (!this->IsCreated())
    {
    return;
    }

  // Use the prev pic, or create a new one

  vtksys_stl::string image_name(
    this->GetColumnConfigurationOption(col_index, "-labelimage"));
  if (!image_name.size())
    {
    char buffer[30];
    sprintf(buffer, ".col_label_img_%d", col_label_img_counter++);
    image_name = this->GetWidgetName();
    image_name += buffer;
    }

  if (!vtkKWTkUtilities::UpdatePhoto(this->GetApplication(),
                                     image_name.c_str(),
                                     pixels, 
                                     width, height, pixel_size,
                                     buffer_length,
                                     this->GetWidgetName(),
                                     "-labelbg"))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " << image_name.c_str());
    return;
    }

  this->SetColumnLabelImage(col_index, image_name.c_str());
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SortByColumn(int col_index, int order)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *sort_opt;
  switch (order)
    {
    case vtkKWMultiColumnList::SortByIncreasingOrder:
      sort_opt = "-increasing";
      break;
    case vtkKWMultiColumnList::SortByDecreasingOrder:
      sort_opt = "-decreasing";
      break;
    default:
      sort_opt = "-increasing";
      break;
    }

  this->Script(
    "%s sortbycolumn %d %s", this->GetWidgetName(), col_index,  sort_opt);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnSortMode(int col_index, int mode)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *sortmode_opt;
  switch (mode)
    {
    case vtkKWMultiColumnList::SortModeAscii:
      sortmode_opt = "ascii";
      break;
    case vtkKWMultiColumnList::SortModeDictionary:
      sortmode_opt = "dictionary";
      break;
    case vtkKWMultiColumnList::SortModeInteger:
      sortmode_opt = "integer";
      break;
    case vtkKWMultiColumnList::SortModeReal:
      sortmode_opt = "real";
      break;
    default:
      sortmode_opt = "ascii";
      break;
    }
  this->SetColumnConfigurationOption(col_index, "-sortmode", sortmode_opt);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnSortMode(int col_index)
{
  if (this->IsCreated())
    {
    const char *val = 
      this->GetColumnConfigurationOption(col_index, "-sortmode");
    if (val && *val)
      {
      if (!strcmp(val, "ascii"))
        {
        return vtkKWMultiColumnList::SortModeAscii;
        }
      if (!strcmp(val, "dictionary"))
        {
        return vtkKWMultiColumnList::SortModeDictionary;
        }
      if (!strcmp(val, "integer"))
        {
        return vtkKWMultiColumnList::SortModeInteger;
        }
      if (!strcmp(val, "real"))
        {
        return vtkKWMultiColumnList::SortModeReal;
        }
      }
    }

  return vtkKWMultiColumnList::SortModeUnknown;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSortArrowVisibility(int arg)
{
  this->SetConfigurationOptionAsInt("-showarrow", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetSortArrowVisibility()
{
  return this->GetConfigurationOptionAsInt("-showarrow");
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetColumnConfigurationOption(
  int col_index, const char *option, const char *value)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  if (!option || !value)
    {
    vtkWarningMacro("Wrong option or value !");
    return 0;
    }

  const char *res = 
    this->Script("%s columnconfigure %d %s {%s}", 
                 this->GetWidgetName(), col_index, option, value);

  // 'configure' is not supposed to return anything, so let's assume
  // any output is an error

  if (res && *res)
    {
    vtksys_stl::string err_msg(res);
    vtksys_stl::string tcl_name(this->GetTclName());
    vtksys_stl::string widget_name(this->GetWidgetName());
    vtksys_stl::string type(this->GetType());
    vtkErrorMacro(
      "Error configuring " << tcl_name.c_str() << " (" << type.c_str() << ": " 
      << widget_name.c_str() << ") at column: " << col_index 
      << " with option: [" << option 
      << "] and value [" << value << "] => " << err_msg.c_str());
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::HasColumnConfigurationOption(
  int col_index, const char *option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  return (this->GetApplication() && 
          !this->GetApplication()->EvaluateBooleanExpression(
            "catch {%s columncget %d %s}",
            this->GetWidgetName(), col_index, option));
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetColumnConfigurationOption(
  int col_index, const char* option)
{
  if (!this->HasColumnConfigurationOption(col_index, option))
    {
    return NULL;
    }

  return this->Script(
    "%s columncget %d %s", this->GetWidgetName(), col_index, option);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetColumnConfigurationOptionAsInt(
  int col_index, const char *option, int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->SetColumnConfigurationOption(col_index, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetColumnConfigurationOptionAsInt(
  int col_index, const char* option)
{
  if (!this->HasColumnConfigurationOption(col_index, option))
    {
    return 0;
    }

  return atoi(
    this->Script("%s columncget %d %s", 
                 this->GetWidgetName(), col_index, option));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnConfigurationOptionAsText(
  int col_index, const char *option, const char *value)
{
  if (!option || !this->IsCreated())
    {
    return;
    }

  const char *val = this->ConvertInternalStringToTclString(
    value, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
  this->Script("%s columnconfigure %d %s \"%s\"", 
               this->GetWidgetName(), col_index, option, val ? val : "");
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetColumnConfigurationOptionAsText(
  int col_index, const char *option)
{
  if (!option || !this->IsCreated())
    {
    return "";
    }

  return this->ConvertTclStringToInternalString(
    this->GetColumnConfigurationOption(col_index, option));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnFormatCommand(int col_index, 
                                                  vtkObject* object, 
                                                  const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->SetColumnConfigurationOption(col_index, "-formatcommand", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnFormatCommandToEmptyOutput(int col_index)
{
  this->SetColumnFormatCommand(col_index, NULL, "tablelist::emptyStr");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetMovableRows(int arg)
{
  this->SetConfigurationOptionAsInt("-movablerows", arg ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetMovableRows()
{
  return this->GetConfigurationOptionAsInt("-movablerows");
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
int vtkKWMultiColumnList::GetNumberOfRows()
{
  if (this->IsCreated())
    {
    const char *val = this->Script("%s size", this->GetWidgetName());
    if (val && *val)
      {
      return atoi(val);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SeeRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s see %d", this->GetWidgetName(), row_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeleteRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s delete %d %d", 
                 this->GetWidgetName(), row_index, row_index);
    }
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
void vtkKWMultiColumnList::GetRowBackgroundColor(
  int row_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetRowConfigurationOption(row_index, "-bg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetRowBackgroundColor(int row_index)
{
  static double rgb[3];
  this->GetRowBackgroundColor(row_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetRowBackgroundColor(
  int row_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetRowConfigurationOption(row_index, "-bg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetRowForegroundColor(
  int row_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetRowConfigurationOption(row_index, "-fg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetRowForegroundColor(int row_index)
{
  static double rgb[3];
  this->GetRowForegroundColor(row_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetRowForegroundColor(
  int row_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetRowConfigurationOption(row_index, "-fg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetStripeBackgroundColor(
  double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-stripebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetStripeBackgroundColor()
{
  static double rgb[3];
  this->GetStripeBackgroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetStripeBackgroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-stripebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetStripeForegroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-stripeforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetStripeForegroundColor()
{
  static double rgb[3];
  this->GetStripeForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetStripeForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-stripeforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetStripeHeight(int height)
{
  this->SetConfigurationOptionAsInt("-stripeheight", height);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetStripeHeight()
{
  return this->GetConfigurationOptionAsInt("-stripeheight");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetRowSelectable(int row_index, int flag)
{
  this->SetRowConfigurationOptionAsInt(row_index, "-selectable", flag ?1:0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetRowSelectable(int row_index)
{
  return this->GetRowConfigurationOptionAsInt(row_index, "-selectable");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ActivateRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s activate %d", this->GetWidgetName(), row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetRowConfigurationOption(
  int row_index, const char *option, const char *value)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  if (!option || !value)
    {
    vtkWarningMacro("Wrong option or value !");
    return 0;
    }

  const char *res = 
    this->Script("%s rowconfigure %d %s {%s}", 
                 this->GetWidgetName(), row_index, option, value);

  // 'configure' is not supposed to return anything, so let's assume
  // any output is an error

  if (res && *res)
    {
    vtksys_stl::string err_msg(res);
    vtksys_stl::string tcl_name(this->GetTclName());
    vtksys_stl::string widget_name(this->GetWidgetName());
    vtksys_stl::string type(this->GetType());
    vtkErrorMacro(
      "Error configuring " << tcl_name.c_str() << " (" << type.c_str() << ": " 
      << widget_name.c_str() << ") at row: " << row_index 
      << " with option: [" << option 
      << "] and value [" << value << "] => " << err_msg.c_str());
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::HasRowConfigurationOption(
  int row_index, const char *option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  return (this->GetApplication() && 
          !this->GetApplication()->EvaluateBooleanExpression(
            "catch {%s rowcget %d %s}",
            this->GetWidgetName(), row_index, option));
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetRowConfigurationOption(
  int row_index, const char* option)
{
  if (!this->HasRowConfigurationOption(row_index, option))
    {
    return NULL;
    }

  return this->Script(
    "%s rowcget %d %s", this->GetWidgetName(), row_index, option);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetRowConfigurationOptionAsInt(
  int row_index, const char *option, int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return this->SetRowConfigurationOption(row_index, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetRowConfigurationOptionAsInt(
  int row_index, const char* option)
{
  if (!this->HasRowConfigurationOption(row_index, option))
    {
    return 0;
    }

  return atoi(
    this->Script("%s rowcget %d %s", 
                 this->GetWidgetName(), row_index, option));
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
void vtkKWMultiColumnList::InsertCellTextAsInt(
  int row_index, int col_index, int value)
{
  char tmp[1024];
  sprintf(tmp, "%d", value);
  this->InsertCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertCellTextAsDouble(
  int row_index, int col_index, double value)
{
  char tmp[1024];
  sprintf(tmp, "%.5g", value);
  this->InsertCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::InsertCellTextAsFormattedDouble(
  int row_index, int col_index, double value, int size)
{
  char format[1024];
  sprintf(format, "%%.%dg", size);
  char tmp[1024];
  sprintf(tmp, format, value);
  this->InsertCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellText(
  int row_index, int col_index, const char *text)
{
  this->SetCellConfigurationOptionAsText(row_index, col_index, "-text", text);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellTextAsInt(
  int row_index, int col_index, int value)
{
  char tmp[1024];
  sprintf(tmp, "%d", value);
  this->SetCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellTextAsDouble(
  int row_index, int col_index, double value)
{
  char tmp[1024];
  sprintf(tmp, "%.5g", value);
  this->SetCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellTextAsFormattedDouble(
  int row_index, int col_index, double value, int size)
{
  char format[1024];
  sprintf(format, "%%.%dg", size);
  char tmp[1024];
  sprintf(tmp, format, value);
  this->SetCellText(row_index, col_index, tmp);
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellText(int row_index, int col_index)
{
  return this->GetCellConfigurationOptionAsText(row_index, col_index, "-text");
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetCellTextAsInt(int row_index, int col_index)
{
  return atoi(this->GetCellConfigurationOption(row_index, col_index, "-text"));
}

//----------------------------------------------------------------------------
double vtkKWMultiColumnList::GetCellTextAsDouble(int row_index, int col_index)
{
  return atof(this->GetCellConfigurationOption(row_index, col_index, "-text"));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::ActivateCell(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s activate %d,%d", 
                 this->GetWidgetName(), row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SeeCell(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s seecell %d,%d", 
                 this->GetWidgetName(), row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetCellBackgroundColor(
  int row_index, int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetCellConfigurationOption(row_index, col_index, "-bg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetCellBackgroundColor(
  int row_index, int col_index)
{
  static double rgb[3];
  this->GetCellBackgroundColor(row_index, col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellBackgroundColor(
  int row_index, int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetCellConfigurationOption(row_index, col_index, "-bg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetCellForegroundColor(
  int row_index, int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetCellConfigurationOption(row_index, col_index, "-fg"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetCellForegroundColor(int row_index, int col_index)
{
  static double rgb[3];
  this->GetCellForegroundColor(row_index, col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellForegroundColor(
  int row_index, int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetCellConfigurationOption(row_index, col_index, "-fg", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetCellCurrentBackgroundColor(
  int row_index, int col_index, double *r, double *g, double *b)
{
  const char *bgcolor;

  // Cell color has priority

  bgcolor =  this->GetCellConfigurationOption(row_index, col_index, "-bg");
  if (bgcolor && *bgcolor)
    {
    this->GetCellBackgroundColor(row_index, col_index, r, g, b);
    return;
    }
  
  // The row color

  bgcolor = this->GetRowConfigurationOption(row_index, "-bg");
  if (bgcolor && *bgcolor)
    {
    this->GetRowBackgroundColor(row_index, r, g, b);
    return;
    }

  // Then stripe color, if within a stripe

  bgcolor = this->GetConfigurationOption("-stripebackground");
  if (bgcolor && *bgcolor)
    {
    int stripeh = this->GetStripeHeight();
    if ((row_index / stripeh) & 1)
      {
      this->GetStripeBackgroundColor(r, g, b);
      return ;
      }
    }
  
  // Then column color

  bgcolor = this->GetColumnConfigurationOption(col_index, "-bg");
  if (bgcolor && *bgcolor)
    {
    this->GetColumnBackgroundColor(col_index, r, g, b);
    return;
    }

  // Then background color

  this->GetBackgroundColor(r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetCellCurrentBackgroundColor(
  int row_index, int col_index)
{
  static double rgb[3];
  this->GetCellCurrentBackgroundColor(
    row_index, col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellEditable(
  int row_index, int col_index, int flag)
{
  this->SetCellConfigurationOptionAsInt(
    row_index, col_index, "-editable", flag ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetCellEditable(
  int row_index, int col_index)
{
  return this->GetCellConfigurationOptionAsInt(
    row_index, col_index, "-editable");
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellImage(
  int row_index, int col_index, const char *image_name)
{
  this->SetCellConfigurationOption(
    row_index, col_index, "-image", image_name);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellImageToPredefinedIcon(
  int row_index, int col_index, int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetCellImageToIcon(row_index, col_index, icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellImageToIcon(
  int row_index, int col_index, vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetCellImageToPixels(
      row_index, col_index, 
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellImageToPixels(
  int row_index, int col_index,
  const unsigned char* pixels, 
  int width, 
  int height,
  int pixel_size,
  unsigned long buffer_length)
{
  static int cell_img_counter = 0;

  if (!this->IsCreated())
    {
    return;
    }

  // Use the prev pic, or create a new one

  vtksys_stl::string image_name(
    this->GetCellConfigurationOption(row_index, col_index, "-image"));
  if (!image_name.size())
    {
    char buffer[30];
    sprintf(buffer, ".cell_img_%d", cell_img_counter++);
    image_name = this->GetWidgetName();
    image_name += buffer;
    }

  // Use the current cell background color to blend

  double r, g, b;
  this->GetCellCurrentBackgroundColor(row_index, col_index, &r, &g, &b);
  char bgcolor[10];
  sprintf(bgcolor, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));

  if (!vtkKWTkUtilities::UpdatePhoto(this->GetApplication(),
                                     image_name.c_str(),
                                     pixels, 
                                     width, height, pixel_size,
                                     buffer_length,
                                     this->GetWidgetName(),
                                     bgcolor))
    {
    vtkWarningMacro(
      << "Error updating Tk photo " << image_name.c_str());
    return;
    }

  this->SetCellImage(row_index, col_index, image_name.c_str());
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
    this->SetCellConfigurationOption(row_index, col_index, "-window", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellWindowWidgetName(int row_index, 
                                                   int col_index)
{
  if (this->IsCreated())
    {
    return this->Script("%s windowpath %d,%d", 
                        this->GetWidgetName(), row_index, col_index);
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddBindingsToWidgetName(const char *widget_name)
{
  if (!this->IsCreated() || !widget_name || !*widget_name)
    {
    return;
    }

  this->Script("bindtags %s [lreplace [bindtags %s] 1 1 TablelistBody]",
               widget_name, widget_name);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddBindingsToWidget(vtkKWWidget *widget)
{
  if (!widget || !widget->IsCreated())
    {
    return;
    }

  this->AddBindingsToWidgetName(widget->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::AddBindingsToWidgetAndChildren(vtkKWWidget *widget)
{
  this->AddBindingsToWidget(widget);
  int nb_children = widget->GetNumberOfChildren();
  for (int i = 0; i < nb_children; i++)
    {
    this->AddBindingsToWidget(widget->GetNthChild(i));
    }
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
int* vtkKWMultiColumnList::FindCellText(const char *text)
{
  static int pos[2];
  if (this->FindCellText(text, pos, pos + 1))
    {
    return pos;
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::FindCellTextInColumn(
  int col_index, const char *text)
{
  if (this->IsCreated() && text)
    {
    int nb_rows = this->GetNumberOfRows();

    for (int j = 0; j < nb_rows; j++)
      {
      const char *cell_text = this->GetCellText(j, col_index);
      if (cell_text && !strcmp(cell_text, text))
        {
        return j;
        }
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::EditCell(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s editcell %d,%d", 
                 this->GetWidgetName(), row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::CancelEditing()
{
  if (this->IsCreated())
    {
    this->Script("%s cancelediting",  this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::RejectInput()
{
  if (this->IsCreated())
    {
    this->Script("%s rejectinput",  this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetCellConfigurationOption(
  int row_index, int col_index, const char *option, const char *value)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  if (!option || !value)
    {
    vtkWarningMacro("Wrong option or value !");
    return 0;
    }

  const char *res = 
    this->Script("%s cellconfigure %d,%d %s {%s}", 
                 this->GetWidgetName(), row_index, col_index, option, value);

  // 'configure' is not supposed to return anything, so let's assume
  // any output is an error

  if (res && *res)
    {
    vtksys_stl::string err_msg(res);
    vtksys_stl::string tcl_name(this->GetTclName());
    vtksys_stl::string widget_name(this->GetWidgetName());
    vtksys_stl::string type(this->GetType());
    vtkErrorMacro(
      "Error configuring " << tcl_name.c_str() << " (" << type.c_str() << ": " 
      << widget_name.c_str() << ") at cell: " << row_index << "," << col_index
      << " with option: [" << option 
      << "] and value [" << value << "] => " << err_msg.c_str());
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::HasCellConfigurationOption(
  int row_index, int col_index, const char *option)
{
  if (!this->IsCreated())
    {
    vtkWarningMacro("Widget is not created yet !");
    return 0;
    }

  return (this->GetApplication() && 
          !this->GetApplication()->EvaluateBooleanExpression(
            "catch {%s cellcget %d,%d %s}",
            this->GetWidgetName(), row_index, col_index, option));
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellConfigurationOption(
  int row_index, int col_index, const char* option)
{
  if (!this->HasCellConfigurationOption(row_index, col_index, option))
    {
    return NULL;
    }

  return this->Script(
    "%s cellcget %d,%d %s", this->GetWidgetName(), row_index,col_index,option);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::SetCellConfigurationOptionAsInt(
  int row_index, int col_index, const char *option, int value)
{
  char buffer[20];
  sprintf(buffer, "%d", value);
  return 
    this->SetCellConfigurationOption(row_index, col_index, option, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetCellConfigurationOptionAsInt(
  int row_index, int col_index, const char* option)
{
  if (!this->HasCellConfigurationOption(row_index, col_index, option))
    {
    return 0;
    }

  return atoi(
    this->Script("%s cellcget %d,%d %s", 
                 this->GetWidgetName(), row_index, col_index, option));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellConfigurationOptionAsText(
  int row_index, int col_index, const char *option, const char *value)
{
  if (!option || !this->IsCreated())
    {
    return;
    }

  const char *val = this->ConvertInternalStringToTclString(
    value, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
  this->Script("%s cellconfigure %d,%d %s \"%s\"", 
               this->GetWidgetName(), 
               row_index, col_index, option, val ? val : "");
}

//----------------------------------------------------------------------------
const char* vtkKWMultiColumnList::GetCellConfigurationOptionAsText(
  int row_index, int col_index, const char *option)
{
  if (!option || !this->IsCreated())
    {
    return "";
    }

  return this->ConvertTclStringToInternalString(
    this->GetCellConfigurationOption(row_index, col_index, option));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetSelectionBackgroundColor(
  double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-selectbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetSelectionBackgroundColor()
{
  static double rgb[3];
  this->GetSelectionBackgroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionBackgroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-selectbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetSelectionForegroundColor(double *r, double *g, double *b)
{
  vtkKWTkUtilities::GetOptionColor(this, "-selectforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetSelectionForegroundColor()
{
  static double rgb[3];
  this->GetSelectionForegroundColor(rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionForegroundColor(double r, double g, double b)
{
  vtkKWTkUtilities::SetOptionColor(this, "-selectforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnSelectionBackgroundColor(
  int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetColumnConfigurationOption(col_index, "-selectbackground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnSelectionBackgroundColor(int col_index)
{
  static double rgb[3];
  this->GetColumnSelectionBackgroundColor(col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnSelectionBackgroundColor(
  int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetColumnConfigurationOption(col_index, "-selectbackground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetColumnSelectionForegroundColor(
  int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetColumnConfigurationOption(col_index, "-selectforeground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetColumnSelectionForegroundColor(int col_index)
{
  static double rgb[3];
  this->GetColumnSelectionForegroundColor(col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetColumnSelectionForegroundColor(
  int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetColumnConfigurationOption(col_index, "-selectforeground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetRowSelectionBackgroundColor(
  int row_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetRowConfigurationOption(row_index, "-selectbackground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetRowSelectionBackgroundColor(int row_index)
{
  static double rgb[3];
  this->GetRowSelectionBackgroundColor(row_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetRowSelectionBackgroundColor(
  int row_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetRowConfigurationOption(row_index, "-selectbackground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetRowSelectionForegroundColor(
  int row_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetRowConfigurationOption(row_index, "-selectforeground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetRowSelectionForegroundColor(int row_index)
{
  static double rgb[3];
  this->GetRowSelectionForegroundColor(row_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetRowSelectionForegroundColor(
  int row_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetRowConfigurationOption(row_index, "-selectforeground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetCellSelectionBackgroundColor(
  int row_index, int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetCellConfigurationOption(
      row_index, col_index, "-selectbackground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetCellSelectionBackgroundColor(
  int row_index, int col_index)
{
  static double rgb[3];
  this->GetCellSelectionBackgroundColor(
    row_index, col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellSelectionBackgroundColor(
  int row_index, int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetCellConfigurationOption(
    row_index, col_index, "-selectbackground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::GetCellSelectionForegroundColor(
  int row_index, int col_index, double *r, double *g, double *b)
{
  vtksys_stl::string color(
    this->GetCellConfigurationOption(
      row_index, col_index, "-selectforeground"));
  vtkKWTkUtilities::GetRGBColor(this, color.c_str(), r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMultiColumnList::GetCellSelectionForegroundColor(
  int row_index, int col_index)
{
  static double rgb[3];
  this->GetCellSelectionForegroundColor(row_index, col_index, rgb, rgb + 1, rgb + 2);
  return rgb;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetCellSelectionForegroundColor(
  int row_index, int col_index, double r, double g, double b)
{
  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(r * 255.0), (int)(g * 255.0), (int)(b * 255.0));
  this->SetCellConfigurationOption(
    row_index, col_index, "-selectforeground", color);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionMode(int relief)
{
  this->SetConfigurationOption(
    "-selectmode", vtkKWTkOptions::GetSelectionModeAsTkOptionValue(relief));
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetSelectionMode()
{
  return vtkKWTkOptions::GetSelectionModeFromTkOptionValue(
    this->GetConfigurationOption("-selectmode"));
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionType(int type)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *type_opt;
  switch (type)
    {
    case vtkKWMultiColumnList::SelectionTypeRow:
      type_opt = "row";
      break;
    case vtkKWMultiColumnList::SelectionTypeCell:
      type_opt = "cell";
      break;
    default:
      type_opt = "row";
      break;
    }
  this->SetConfigurationOption("-selecttype", type_opt);
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::GetSelectionType()
{
  if (this->IsCreated())
    {
    const char *val = this->GetConfigurationOption("-selecttype");
    if (val && *val)
      {
      if (!strcmp(val, "row"))
        {
        return vtkKWMultiColumnList::SelectionTypeRow;
        }
      if (!strcmp(val, "cell"))
        {
        return vtkKWMultiColumnList::SelectionTypeCell;
        }
      }
    }

  return vtkKWMultiColumnList::SelectionTypeUnknown;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectSingleRow(int row_index)
{
  this->ClearSelection();
  this->SelectRow(row_index);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s selection set %d %d", 
                 this->GetWidgetName(), row_index, row_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeselectRow(int row_index)
{
  if (this->IsCreated())
    {
    this->Script("%s selection clear %d %d", 
                 this->GetWidgetName(), row_index, row_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::IsRowSelected(int row_index)
{
  if (this->IsCreated())
    {
    return atoi(this->Script("%s selection includes %d", 
                             this->GetWidgetName(), row_index));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectSingleCell(int row_index, int col_index)
{
  this->ClearSelection();
  this->SelectCell(row_index, col_index);
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SelectCell(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s cellselection set %d,%d %d,%d", 
                 this->GetWidgetName(), 
                 row_index, col_index, row_index, col_index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::DeselectCell(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    this->Script("%s cellselection clear %d,%d %d,%d", 
                 this->GetWidgetName(), 
                 row_index, col_index, row_index, col_index);
    }
}

//----------------------------------------------------------------------------
int vtkKWMultiColumnList::IsCellSelected(int row_index, int col_index)
{
  if (this->IsCreated())
    {
    return atoi(this->Script("%s cellselection includes %d,%d", 
                             this->GetWidgetName(), row_index, col_index));
    }
    return 0;
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
void vtkKWMultiColumnList::ClearSelection()
{
  if (this->IsCreated())
    {
    this->Script("%s selection clear 0 end", this->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::SetSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionChangedCommand, object, method);
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
void vtkKWMultiColumnList::SetEditStartCommand(vtkObject* object, 
                                               const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->SetConfigurationOption("-editstartcommand", command);
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
    this->SetConfigurationOption("-editendcommand", command);
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
    this->SetConfigurationOption("-labelcommand", command);
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
    this->SetConfigurationOption("-sortcommand", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWMultiColumnList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
