/*=========================================================================

  Module:    vtkKWMultiColumnList.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMultiColumnList - a multi-column list
// .SECTION Description
// A composite widget used for displaying multi-column lists. It is a
// front-end to a tablelist::tablelist.
// A tablelist is a multi-column listbox, implemented as a mega-widget, 
// consisting of a body and a header. The body displays a list of items, one
// per line. Each item is a list of elements, which are aligned in columns. 
// In other words, an item is the contents of a row, and an element is the
// text contained in a cell. The header consists of label widgets displaying 
// the column titles. The labels can be used, among others, for interactive
// column resizing and column-based sorting of the items.

#ifndef __vtkKWMultiColumnList_h
#define __vtkKWMultiColumnList_h

#include "vtkKWWidget.h"

class KWWIDGETS_EXPORT vtkKWMultiColumnList : public vtkKWWidget
{
public:
  static vtkKWMultiColumnList* New();
  vtkTypeRevisionMacro(vtkKWMultiColumnList,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget
  // This will create a frame, inside which a 'tablelist' widget
  // will be packed. Use GetTableList() to access/configure it.
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the width (in chars) and height (in lines).
  virtual void SetWidth(int width);
  virtual int GetWidth();
  virtual void SetHeight(int height);
  virtual int GetHeight();

  // Description:
  // Use vertical scrollbar (default to On).
  virtual void SetUseVerticalScrollbar(int val);
  vtkGetMacro(UseVerticalScrollbar, int);
  vtkBooleanMacro(UseVerticalScrollbar, int);

  // Description:
  // Use horizontal scrollbar (default to On).
  virtual void SetUseHorizontalScrollbar(int val);
  vtkGetMacro(UseHorizontalScrollbar, int);
  vtkBooleanMacro(UseHorizontalScrollbar, int);

  // Description:
  // Add a column at the end.
  // Optional width must be a number. A positive value specifies the column's
  // width in average-size characters of the widget's font.  If width is
  // negative, its absolute value is interpreted as a column width in pixels.
  // Finally, a value of zero (default) specifies that the column's width is
  // to be made just large enough to hold all the elements in the column, 
  // including its header
  // Optional alignment specifies how to align the elements of the column.
  // Each alignment must be one of left (default), right, or center.  
  //BTX
  enum 
  {
    COLUMN_ALIGNMENT_LEFT,
    COLUMN_ALIGNMENT_RIGHT,
    COLUMN_ALIGNMENT_CENTER
  };
  //ETX
  virtual void AddColumn(
    const char *title, int width = 0, int align = COLUMN_ALIGNMENT_LEFT);

  // Description:
  // Set the width and maxwidth of a column.
  // Both must be a number. A positive value specifies the column's
  // width in average-size characters of the widget's font.  If width is
  // negative, its absolute value is interpreted as a column width in pixels.
  // Finally, a value of zero (default) specifies that the column's width is
  // to be made just large enough to hold all the elements in the column, 
  // including its header
  virtual void SetColumnWidth(int col_index, int width);
  virtual int GetColumnWidth(int col_index);
  virtual void SetColumnMaximumWidth(int col_index, int width);
  virtual int GetColumnMaximumWidth(int col_index);

  // Description:
  // Get number columns.
  // Returns -1 on error.
  virtual int GetNumberOfColumns();

  // Description:
  // Sort by a given column.
  virtual void SortByColumn(int col_index, int increasing_order = 1);

  // Description:
  // Specifies a boolean value that determines whether the columns can be 
  // moved interactively.
  vtkBooleanMacro(MovableColumns, int);
  virtual void SetMovableColumns(int);
  virtual int GetMovableColumns();

  // Description:
  // Specifies a boolean value that determines whether the columns can be 
  // resized interactively.
  vtkBooleanMacro(ResizableColumns, int);
  virtual void SetResizableColumns(int);
  virtual int GetResizableColumns();

  // Description:
  // Specifies a boolean value that determines whether a specific column 
  // can be resized interactively.
  virtual void SetColumnResizable(int col_index, int flag);
  virtual void ColumnResizableOn(int col_index)
    { this->SetColumnResizable(col_index, 1); };
  virtual void ColumnResizableOff(int col_index)
    { this->SetColumnResizable(col_index, 0); };
  virtual int GetColumnResizable(int col_index);

  // Description:
  // Specifies a boolean value that determines whether a specific column 
  // can be edited interactively.
  virtual void SetColumnEditable(int col_index, int flag);
  virtual void ColumnEditableOn(int col_index)
    { this->SetColumnEditable(col_index, 1); };
  virtual void ColumnEditableOff(int col_index)
    { this->SetColumnEditable(col_index, 0); };
  virtual int GetColumnEditable(int col_index);

  // Description:
  // Specifies a boolean value that determines whether the columns are to be
  // separated with borders.
  vtkBooleanMacro(ShowSeparators, int);
  virtual void SetShowSeparators(int);
  virtual int GetShowSeparators();

  // Description:
  // Configure the options of a column.
  // Check the tablelist::tablelist manual for valid options
  // Ex: -align -background -editable -editwindow -font -foreground -hide
  // -maxwidth -resizable -title -width width, etc.
  virtual void ConfigureColumnOptions(int col_index, const char *options);

  // Description:
  // Add a row at the end, or insert it at a given location.
  virtual void InsertRow(int row_index);
  virtual void AddRow();

  // Description:
  // Clear selection
  virtual void ClearSelection();

  // Description:
  // Select a single row (any other selection is cleared).
  virtual void SelectSingleRow(int row_index);

  // Description:
  // Get index of first selected row.
  // Returns -1 on error.
  virtual int GetIndexOfFirstSelectedRow();

  // Description:
  // Specifies a command to be invoked when the selection has changed
  virtual void SetSelectionChangedCommand(
    vtkKWObject* object, const char *method);

  // Description:
  // Active a row.
  virtual void ActivateRow(int row_index);

  // Description:
  // Get number of rows.
  // Returns -1 on error.
  virtual int GetNumberOfRows();

  // Description:
  // Delete all rows in the list.
  virtual void DeleteAllRows();

  // Description:
  // Configure the options of a row.
  // Check the tablelist::tablelist manual for valid options
  // Ex: -background -font -foreground -selectable -selectbackground 
  // -selectforeground -text
  virtual void ConfigureRowOptions(int row_index, const char *options);

  // Description:
  // Set/Get contents of cell (warning, Get returns a pointer to the Tcl
  // buffer, copy the resulting string ASAP).
  // SetCellText is the fast version and assumes the cell already exists!
  // InsertCellText will insert one (or more) full row(s) if there is no
  // row/cell at that location (using InsertRow).
  virtual void InsertCellText(int row_index, int col_index, const char *text);
  virtual void SetCellText(int row_index, int col_index, const char *text);
  virtual const char* GetCellText(int row_index, int col_index);

  // Description:
  // Find contents of cell and return its location
  // Return 1 if found, 0 otherwise
  virtual int FindCellText(const char *text, int *row_index, int *col_index);
  virtual int FindCellTextInColumn(int col_index, const char *text, int *row_index);

  // Description:
  // Edit cell.  If supported, edit cell contents interactively
  virtual void EditCellText(int row_index, int col_index);

  // Description:
  // Configure the options of a cell.
  // Check the tablelist::tablelist manual for valid options
  // Ex: -background -editable -font -foreground -unage -window 
  // -selectbackground  -selectforeground -text
  virtual void ConfigureCellOptions(
    int row_index, int col_index, const char *options);

  // Description:
  // Specifies a Tcl command creating the window to be embedded into a cell.
  // The command is automatically concatenated with the name of the tablelist
  // widget, the cell's row and column indices, as well as the path name of
  // the embedded window to be created, and the resulting script is evaluated
  // in the global scope.
  virtual void SetCellWindowCommand(int row_index, int col_index, 
                                    vtkKWObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the interactive editing of a cell's
  // contents is started. The command is automatically concatenated with the
  // name of the tablelist widget (TableList), the cell's row and column
  // indices, as well as the text displayed in the cell, the resulting script
  // is evaluated in the global scope, and the return value becomes the 
  // initial contents of the temporary embedded widget used for the editing.
  virtual void SetEditStartCommand(vtkKWObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked on normal termination of the 
  // interactive editing of a cell's contents if the final text of the
  // temporary embedded widget used for the editing is different from its 
  // initial one. The command is automatically concatenated with the name of 
  // the tablelist widget, the cell's row and column indices, as well as the
  // final contents of the edit window, the resulting script is evaluated in
  // the global scope, and the return value becomes the cell's new contents 
  // after destroying the temporary embedded widget. The main purpose of this
  // script is to perform a final validation of the edit window's contents.
  virtual void SetEditEndCommand(vtkKWObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when mouse button 1 is pressed over one
  // of the header labels and later released over the same label. When the
  // <ButtonRelease-1> event occurs, the command is automatically 
  // concatenated with the name of the tablelist widget and the column index
  // of the respective label, and the resulting script is evaluated in the
  // global scope. 
  virtual void SetLabelCommand(vtkKWObject* object, const char *method);

  // Description:
  // Specifies a command to be used for the comparison of the items when
  // invoking the sort subcommand of the Tcl command associated with the
  // tablelist widget. To compare two items (viewed as lists of cell contents
  // within one row each) during the sort operation, the command is 
  // automatically concatenated with the two items and the resulting script
  // is evaluated. The script should return an integer less than, equal to, or
  // greater than zero if the first item is to be considered less than, equal
  // to, or greater than the second, respectively.
  virtual void SetSortCommand(vtkKWObject* object, const char *method);

  // Description:
  // Access the internal 'tablelist' widget and the scrollbars.
  vtkGetObjectMacro(TableList, vtkKWWidget);
  vtkGetObjectMacro(VerticalScrollBar, vtkKWWidget);
  vtkGetObjectMacro(HorizontalScrollBar, vtkKWWidget);

  // Description:
  // Callbacks
  virtual void SelectionChangedCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWMultiColumnList();
  ~vtkKWMultiColumnList();

  vtkKWWidget *TableList;
  vtkKWWidget *VerticalScrollBar;
  vtkKWWidget *HorizontalScrollBar;

  int UseVerticalScrollbar;
  int UseHorizontalScrollbar;

  char  *SelectionChangedCommand;

  // Description:
  // Create scrollbars
  virtual void CreateHorizontalScrollbar(vtkKWApplication *app);
  virtual void CreateVerticalScrollbar(vtkKWApplication *app);

  // Description:
  // Pack.
  virtual void Pack();

private:
  vtkKWMultiColumnList(const vtkKWMultiColumnList&); // Not implemented
  void operator=(const vtkKWMultiColumnList&); // Not implemented
};

#endif
