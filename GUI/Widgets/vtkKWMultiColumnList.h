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
// Use vtkKWMultiColumnListWithScrollbars if you need scrollbars.
// .SECTION See Also
// vtkKWMultiColumnListWithScrollbars

#ifndef __vtkKWMultiColumnList_h
#define __vtkKWMultiColumnList_h

#include "vtkKWCoreWidget.h"

class vtkKWIcon;

class KWWIDGETS_EXPORT vtkKWMultiColumnList : public vtkKWCoreWidget
{
public:
  static vtkKWMultiColumnList* New();
  vtkTypeRevisionMacro(vtkKWMultiColumnList,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set the width (in chars) and height (in lines).
  // If width is set to 0, the widget will be large enough to show
  // all columns. If set to a different value, columns will stretch
  // depending on their width (see SetColumnWidth) and on the strech
  // parameter (see SetColumnStretchable and StretchableColumns)
  virtual void SetWidth(int width);
  virtual int GetWidth();
  virtual void SetHeight(int height);
  virtual int GetHeight();

  // Description:
  // Add a column at the end.
  // Returns the index of the column
  virtual int AddColumn(const char *title);

  // Description:
  // Get number columns.
  // Returns -1 on error.
  virtual int GetNumberOfColumns();

  // Description:
  // Adjusts the view in the tablelist so that the column is visible.
  virtual void SeeColumn(int col_index);

  // Description:
  // Delete one or all columns in the list.
  virtual void DeleteColumn(int col_index);
  virtual void DeleteAllColumns();

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
  // Specifies a boolean value that determines whether the columns are to be
  // separated with borders.
  vtkBooleanMacro(ColumnSeparatorsVisibility, int);
  virtual void SetColumnSeparatorsVisibility(int);
  virtual int GetColumnSeparatorsVisibility();

  // Description:
  // Specifies a boolean value that determines whether the columns labels
  // are to be shown.
  vtkBooleanMacro(ColumnLabelsVisibility, int);
  virtual void SetColumnLabelsVisibility(int);
  virtual int GetColumnLabelsVisibility();

  // Description:
  // Set/Get the column label background and foreground colors.
  virtual void GetColumnLabelBackgroundColor(double *r, double *g, double *b);
  virtual double* GetColumnLabelBackgroundColor();
  virtual void SetColumnLabelBackgroundColor(double r, double g, double b);
  virtual void SetColumnLabelBackgroundColor(double rgb[3])
    { this->SetColumnLabelBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetColumnLabelForegroundColor(double *r, double *g, double *b);
  virtual double* GetColumnLabelForegroundColor();
  virtual void SetColumnLabelForegroundColor(double r, double g, double b);
  virtual void SetColumnLabelForegroundColor(double rgb[3])
    { this->SetColumnLabelForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the column title.
  virtual void SetColumnTitle(int col_index, const char*);
  virtual const char* GetColumnTitle(int col_index);

  // Description:
  // Set/Get the width and maxwidth of a column.
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
  // Specifies a boolean value that determines whether a specific column 
  // can be stretched or not to fill the empty space on the right of
  // the table that might appear when it is resized or the width is
  // set to a larger value (see SetWidth).
  // Use StretchableColumns to set all columns to be stretchable.
  virtual void SetColumnStretchable(int col_index, int flag);
  virtual void ColumnStretchableOn(int col_index)
    { this->SetColumnStretchable(col_index, 1); };
  virtual void ColumnStretchableOff(int col_index)
    { this->SetColumnStretchable(col_index, 0); };
  virtual int GetColumnStretchable(int col_index);
  vtkBooleanMacro(StretchableColumns, int);
  virtual void SetStretchableColumns(int);

  // Description:
  // Set/Get the alignment of a column, or the aligment of the column
  // label specifically.
  // The alignment must be one of left (default), right, or center.  
  //BTX
  enum 
  {
    ColumnAlignmentLeft = 0,
    ColumnAlignmentRight,
    ColumnAlignmentCenter,
    ColumnAlignmentUnknown
  };
  //ETX
  virtual int GetColumnAlignment(int col_index);
  virtual void SetColumnAlignment(int col_index, int align);
  virtual void SetColumnAlignmentToLeft(int col_index)
    { this->SetColumnAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentLeft); };
  virtual void SetColumnAlignmentToRight(int col_index)
    { this->SetColumnAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentRight); };
  virtual void SetColumnAlignmentToCenter(int col_index)
    { this->SetColumnAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentCenter); };
  virtual int GetColumnLabelAlignment(int col_index);
  virtual void SetColumnLabelAlignment(int col_index, int align);
  virtual void SetColumnLabelAlignmentToLeft(int col_index)
    { this->SetColumnLabelAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentLeft); };
  virtual void SetColumnLabelAlignmentToRight(int col_index)
    { this->SetColumnLabelAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentRight); };
  virtual void SetColumnLabelAlignmentToCenter(int col_index)
    { this->SetColumnLabelAlignment(
      col_index, vtkKWMultiColumnList::ColumnAlignmentCenter); };

  // Description:
  // Sort by a given column.
  //BTX
  enum 
  {
    SortByIncreasingOrder = 0,
    SortByDecreasingOrder,
    SortByUnknownOrder
  };
  //ETX
  virtual void SortByColumn(int col_index, int order);
  virtual void SortByColumnIncreasingOrder(int col_index)
    { this->SortByColumn(
      col_index, vtkKWMultiColumnList::SortByIncreasingOrder); };
  virtual void SortByColumnDecreasingOrder(int col_index)
    { this->SortByColumn(
      col_index, vtkKWMultiColumnList::SortByDecreasingOrder); };

  // Description:
  // Set/Get each column sort mode
  //BTX
  enum 
  {
    SortModeAscii = 0,
    SortModeDictionary,
    SortModeInteger,
    SortModeReal,
    SortModeUnknown
  };
  //ETX
  virtual int GetColumnSortMode(int col_index);
  virtual void SetColumnSortMode(int col_index, int mode);
  virtual void SetColumnSortModeToAscii(int col_index)
    { this->SetColumnSortMode(
      col_index, vtkKWMultiColumnList::SortModeAscii); };
  virtual void SetColumnSortModeToDictionary(int col_index)
    { this->SetColumnSortMode(
      col_index, vtkKWMultiColumnList::SortModeDictionary); };
  virtual void SetColumnSortModeToInteger(int col_index)
    { this->SetColumnSortMode(
      col_index, vtkKWMultiColumnList::SortModeInteger); };
  virtual void SetColumnSortModeToReal(int col_index)
    { this->SetColumnSortMode(
      col_index, vtkKWMultiColumnList::SortModeReal); };

  // Description:
  // Specifies a boolean value that determines whether the widget should place
  // an arrow indicating the sort order into the header label of the column
  // being sorted
  vtkBooleanMacro(SortArrowVisibility, int);
  virtual void SetSortArrowVisibility(int);
  virtual int GetSortArrowVisibility();

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
  // Specifies a boolean value that determines whether a specific column 
  // is visible or not.
  virtual void SetColumnVisibility(int col_index, int flag);
  virtual void ColumnVisibilityOn(int col_index)
    { this->SetColumnVisibility(col_index, 1); };
  virtual void ColumnVisibilityOff(int col_index)
    { this->SetColumnVisibility(col_index, 0); };
  virtual int GetColumnVisibility(int col_index);

  // Description:
  // Set/Get a column background and foreground colors
  virtual void GetColumnBackgroundColor(
    int col_index, double *r, double *g, double *b);
  virtual double* GetColumnBackgroundColor(int col_index);
  virtual void SetColumnBackgroundColor(
    int col_index, double r, double g, double b);
  virtual void SetColumnBackgroundColor(int col_index, double rgb[3])
    { this->SetColumnBackgroundColor(col_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetColumnForegroundColor(
    int col_index, double *r, double *g, double *b);
  virtual double* GetColumnForegroundColor(int col_index);
  virtual void SetColumnForegroundColor(
    int col_index, double r, double g, double b);
  virtual void SetColumnForegroundColor(int col_index, double rgb[3])
    { this->SetColumnForegroundColor(col_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Specifies an image to display in the label of a column
  virtual void SetColumnLabelImage(int col_index, const char *);
  virtual void SetColumnLabelImageToIcon(int col_index, vtkKWIcon *icon);
  virtual void SetColumnLabelImageToPredefinedIcon(
    int col_index, int icon_index);
  virtual void SetColumnLabelImageToPixels(
    int col_index, 
    const unsigned char *pixels, int width, int height, int pixel_size,
    unsigned long buffer_length = 0);

  // Description:
  // Specifies the Tcl command to be invoked when displaying the contents 
  // of a cell within this column or adding them to the selection when the
  // latter is being exported. If command is a nonempty string, then it is
  // automatically concatenated with the cell's text, the resulting script is
  // evaluated in the global scope, and the return value is displayed in the
  // cell or added to the selection instead of the original data. For example,
  // a cell may hold a data value in seconds, but the format command could
  // be set to display the cell value as a formatted data (say "%Y-%m-%d").
  // Notice that this option is only used for preparing the text to be
  // displayed or returned when exporting the selection, and does not affect
  // the internal cell contents. In the case of the above example, this will
  // make it possible to sort the items very easily by time, with a second's
  // precision, even if their visual representation only contains the year, 
  // month, and day. 
  // This command also comes in handy if only images or embedded windows are
  // to be displayed in a column but the texts associated with the cells may
  // not simply be empty strings because they are needed for other purposes
  // (like sorting or editing). In such cases, a command returning an empty
  // string can be used, thus making sure that the textual information 
  // contained in that column remains hidden. The special 
  // SetColumnFormatCommandToEmptyOutput can be used for that.
  virtual void SetColumnFormatCommand(int col_index, 
                                      vtkObject* object, const char *method);
  virtual void SetColumnFormatCommandToEmptyOutput(int col_index);

  // Description:
  // Specifies a boolean value that determines whether the rows can be 
  // moved interactively.
  vtkBooleanMacro(MovableRows, int);
  virtual void SetMovableRows(int);
  virtual int GetMovableRows();

  // Description:
  // Add a row at the end, or insert it at a given location.
  virtual void InsertRow(int row_index);
  virtual void AddRow();

  // Description:
  // Get number of rows.
  // Returns -1 on error.
  virtual int GetNumberOfRows();

  // Description:
  // Adjusts the view in the tablelist so that the row is visible.
  virtual void SeeRow(int row_index);

  // Description:
  // Delete one or all rows in the list.
  virtual void DeleteRow(int row_index);
  virtual void DeleteAllRows();

  // Description:
  // Set/Get a row background and foreground colors
  virtual void GetRowBackgroundColor(
    int row_index, double *r, double *g, double *b);
  virtual double* GetRowBackgroundColor(int row_index);
  virtual void SetRowBackgroundColor(
    int row_index, double r, double g, double b);
  virtual void SetRowBackgroundColor(int row_index, double rgb[3])
    { this->SetRowBackgroundColor(row_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetRowForegroundColor(
    int row_index, double *r, double *g, double *b);
  virtual double* GetRowForegroundColor(int row_index);
  virtual void SetRowForegroundColor(
    int row_index, double r, double g, double b);
  virtual void SetRowForegroundColor(int row_index, double rgb[3])
    { this->SetRowForegroundColor(row_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the stripe background and foreground colors, and strip height
  // Specifies the colors to use when displaying the items belonging to a
  // stripe. Each stripe is composed of the same number StripeHeight of
  // consecutive items. The first stripeHeight items are "normal" ones; they 
  // are followed by a stripe composed of the next StripeHeight items, which
  // in turn is followed by the same number of "normal" items, and so on. 
  // The default value is an empty string, indicating that the stripes will
  // inherit the colors of the widget. The Stripe colors have a higher
  // priority than the column colors, but a lower priority than the
  // row or cell color.
  virtual void GetStripeBackgroundColor(double *r, double *g, double *b);
  virtual double* GetStripeBackgroundColor();
  virtual void SetStripeBackgroundColor(double r, double g, double b);
  virtual void SetStripeBackgroundColor(double rgb[3])
    { this->SetStripeBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetStripeForegroundColor(double *r, double *g, double *b);
  virtual double* GetStripeForegroundColor();
  virtual void SetStripeForegroundColor(double r, double g, double b);
  virtual void SetStripeForegroundColor(double rgb[3])
    { this->SetStripeForegroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetStripeHeight(int height);
  virtual int GetStripeHeight();
  
  // Description:
  // Specifies a boolean value that determines whether a specific row 
  // can be selected interactively.
  virtual void SetRowSelectable(int row_index, int flag);
  virtual void RowSelectableOn(int row_index)
    { this->SetRowSelectable(row_index, 1); };
  virtual void RowSelectableOff(int row_index)
    { this->SetRowSelectable(row_index, 0); };
  virtual int GetRowSelectable(int row_index);

  // Description:
  // Activate a row.
  virtual void ActivateRow(int row_index);

  // Description:
  // Set/Get contents of cell (warning, Get returns a pointer to the Tcl
  // buffer, copy the resulting string ASAP).
  // SetCellText is the fast version and assumes the cell already exists!
  // InsertCellText will insert one (or more) full row(s) if there is no
  // row/cell at that location (using InsertRow).
  virtual void InsertCellText(
    int row_index, int col_index, const char *text);
  virtual void InsertCellTextAsInt(
    int row_index, int col_index, int value);
  virtual void InsertCellTextAsDouble(
    int row_index, int col_index, double value);
  virtual void InsertCellTextAsFormattedDouble(
    int row_index, int col_index, double value, int size);
  virtual void SetCellText(
    int row_index, int col_index, const char *text);
  virtual void SetCellTextAsInt(
    int row_index, int col_index, int value);
  virtual void SetCellTextAsDouble(
    int row_index, int col_index, double value);
  virtual void SetCellTextAsFormattedDouble(
    int row_index, int col_index, double value, int size);
  virtual const char* GetCellText(int row_index, int col_index);
  virtual int GetCellTextAsInt(int row_index, int col_index);
  virtual double GetCellTextAsDouble(int row_index, int col_index);

  // Description:
  // Convenience method to set the contents of a full row or full column.
  virtual void InsertRowText(int row_index, const char *text);
  virtual void InsertColumnText(int col_index, const char *text);

  // Description:
  // Convenience method to set the contents of the cell given a column index
  // only and a text to look for in a specific column. If that text is
  // found, its row index is used to set the contents of the cell.
  virtual void FindAndInsertCellText(
    int look_for_col_index, const char *look_for_text , 
    int col_index, const char *text);

  // Description:
  // Activate a cell.
  virtual void ActivateCell(int row_index, int col_index);

  // Description:
  // Adjusts the view in the tablelist so that the cell is visible.
  virtual void SeeCell(int row_index, int col_index);

  // Description:
  // Set/Get a cell background and foreground colors
  virtual void GetCellBackgroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellBackgroundColor(int row_index, int col_index);
  virtual void SetCellBackgroundColor(
    int row_index, int col_index, double r, double g, double b);
  virtual void SetCellBackgroundColor(
    int row_index, int col_index, double rgb[3])
    { this->SetCellBackgroundColor(
      row_index, col_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetCellForegroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellForegroundColor(int row_index, int col_index);
  virtual void SetCellForegroundColor(
    int row_index, int col_index, double r, double g, double b);
  virtual void SetCellForegroundColor(
    int row_index, int col_index, double rgb[3])
    { this->SetCellForegroundColor(
      row_index, col_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Get a the current cell background color
  // In order of priority: cell > row > stripe > column > background color.
  virtual void GetCellCurrentBackgroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellCurrentBackgroundColor(int row_index, int col_index);

  // Description:
  // Specifies a boolean value that determines whether a specific row 
  // can be edited interactively.
  virtual void SetCellEditable(int row_index, int col_index, int flag);
  virtual void CellEditableOn(int row_index, int col_index)
    { this->SetCellEditable(row_index, col_index, 1); };
  virtual void CellEditableOff(int row_index, int col_index)
    { this->SetCellEditable(row_index, col_index, 0); };
  virtual int GetCellEditable(int row_index, int col_index);

  // Description:
  // Specifies an image to display in the cell. Both text and image can
  // be displayed simultaneously. If a WindowCommand is specified for that
  // cell, it overrides the image.
  // An attempt is made to blend the image with the current cell background
  // color (as returned by GetCellBackgroundColor). But since resorting
  // a column, or inserting new rows, can change the position of the cell
  // in a stripe (see SetStripeBackgroundColor), it is best to:
  //   - use images that do not have an alpha component, or 
  //   - set the cell image once all rows have been inserted,
  //   - refresh the image periodically (or each time a row is added/removed)
  virtual void SetCellImage(int row_index, int col_index, const char *);
  virtual void SetCellImageToIcon(
    int row_index, int col_index, vtkKWIcon *icon);
  virtual void SetCellImageToPredefinedIcon(
    int row_index, int col_index, int icon_index);
  virtual void SetCellImageToPixels(
    int row_index, int col_index,
    const unsigned char *pixels, int width, int height, int pixel_size,
    unsigned long buffer_length = 0);

  // Description:
  // Specifies a Tcl command to create the window (i.e. widget) to be embedded
  // into a cell.
  // The command is automatically concatenated with the name of the tablelist
  // widget, the cell's row and column indices, as well as the path name of
  // the embedded window to be created, and the resulting script is evaluated
  // in the global scope.
  // The path of the window contained in the cell as created by that
  // command can be retrieved using GetCellWindowWidgetName.
  // Note that once the widget is created by the command, clicking on it
  // is likely not to trigger the same interactive behavior as clicking
  // on a regular cell (i.e., clicking on the widget will not select the
  // row or cell for example). This can be a good thing if clicking on
  // the widget is meant to be intercepted by the widget to trigger a
  // different behaviour. In other cases, one would want the interaction 
  // bindings to remain the same. In order to do so, these bindings
  // have to be added to the widget the was just created. To do so, 
  // call AddBindingsToWidget, either on the Tk widget name, or on a
  // vtkKWWidget that may have been used to wrap around that Tk widget name.
  // A complex widget can be made of several other sub-widgets that need
  // the bindings to be passed on too. Use AddBindingsToWidgetAndChildren
  // to pass the bindings to a widget and its chilren automatically (or
  // call AddBindingsToWidget manually on each sub-widgets).
  virtual void SetCellWindowCommand(int row_index, int col_index, 
                                    vtkObject* object, const char *method);
  virtual const char* GetCellWindowWidgetName(int row_index, int col_index);
  virtual void AddBindingsToWidgetName(const char *widget_name);
  virtual void AddBindingsToWidget(vtkKWWidget *widget);
  virtual void AddBindingsToWidgetAndChildren(vtkKWWidget *widget);

  // Description:
  // Find contents of cell in all table or single row
  // One FindCellText signature returns 1 if found, 0 otherwise, and
  // assign the position to row_index, col_index. The other FindCellText
  // method returns a pointer to an array of 2 ints (row and col index) if
  // found, NULL otherwise.
  // FindCellTextInColumn return the row index of the cell in the col_index
  // column if found, or -1 otherwise.
  virtual int FindCellText(const char *text, int *row_index, int *col_index);
  virtual int* FindCellText(const char *text);
  virtual int FindCellTextInColumn(int col_index, const char *text);

  // Description:
  // Edit cell (or cancel edit). If supported, edit cell contents interactively
  virtual void EditCell(int row_index, int col_index);
  virtual void CancelEditing();

  // Description:
  // If invoked from within EditEndCommand, then this method prevents the
  // termination of the interactive editing of the contents of a cell.  It
  // enables you to reject the widget's text during the final validation of the
  // string intended to become the new cell contents.
  virtual void RejectInput();

  // Description:
  // Set/Get the selection background and foreground colors.
  virtual void GetSelectionBackgroundColor(double *r, double *g, double *b);
  virtual double* GetSelectionBackgroundColor();
  virtual void SetSelectionBackgroundColor(double r, double g, double b);
  virtual void SetSelectionBackgroundColor(double rgb[3])
    { this->SetSelectionBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void GetSelectionForegroundColor(double *r, double *g, double *b);
  virtual double* GetSelectionForegroundColor();
  virtual void SetSelectionForegroundColor(double r, double g, double b);
  virtual void SetSelectionForegroundColor(double rgb[3])
    { this->SetSelectionForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the selection background and foreground colors for a specific
  // column.
  virtual void GetColumnSelectionBackgroundColor(
    int col_index, double *r, double *g, double *b);
  virtual double* GetColumnSelectionBackgroundColor(int col_index);
  virtual void SetColumnSelectionBackgroundColor(
    int col_index, double r, double g, double b);
  virtual void SetColumnSelectionBackgroundColor(int col_index, double rgb[3])
    { this->SetColumnSelectionBackgroundColor(
      col_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetColumnSelectionForegroundColor(
    int col_index, double *r, double *g, double *b);
  virtual double* GetColumnSelectionForegroundColor(int col_index);
  virtual void SetColumnSelectionForegroundColor(
    int col_index, double r, double g, double b);
  virtual void SetColumnSelectionForegroundColor(int col_index, double rgb[3])
    { this->SetColumnSelectionForegroundColor(
      col_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the selection background and foreground colors for a specific
  // row.
  virtual void GetRowSelectionBackgroundColor(
    int row_index, double *r, double *g, double *b);
  virtual double* GetRowSelectionBackgroundColor(int row_index);
  virtual void SetRowSelectionBackgroundColor(
    int row_index, double r, double g, double b);
  virtual void SetRowSelectionBackgroundColor(int row_index, double rgb[3])
    { this->SetRowSelectionBackgroundColor(
      row_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetRowSelectionForegroundColor(
    int row_index, double *r, double *g, double *b);
  virtual double* GetRowSelectionForegroundColor(int row_index);
  virtual void SetRowSelectionForegroundColor(
    int row_index, double r, double g, double b);
  virtual void SetRowSelectionForegroundColor(int row_index, double rgb[3])
    { this->SetRowSelectionForegroundColor(
      row_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the selection background and foreground colors for a specific
  // cell.
  virtual void GetCellSelectionBackgroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellSelectionBackgroundColor(
    int row_index, int col_index);
  virtual void SetCellSelectionBackgroundColor(
    int row_index, int col_index, double r, double g, double b);
  virtual void SetCellSelectionBackgroundColor(
    int row_index, int col_index, double rgb[3])
    { this->SetCellSelectionBackgroundColor(
      row_index, col_index, rgb[0], rgb[1], rgb[2]); };
  virtual void GetCellSelectionForegroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellSelectionForegroundColor(
    int row_index, int col_index);
  virtual void SetCellSelectionForegroundColor(
    int row_index, int col_index, double r, double g, double b);
  virtual void SetCellSelectionForegroundColor(
    int row_index, int col_index, double rgb[3])
    { this->SetCellSelectionForegroundColor(
      row_index, col_index, rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the one of several styles for manipulating the selection. 
  // Valid constants can be found in vtkKWTkOptions::SelectionModeType.
  virtual void SetSelectionMode(int);
  virtual int GetSelectionMode();
  virtual void SetSelectionModeToSingle() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeSingle); };
  virtual void SetSelectionModeToBrowse() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeBrowse); };
  virtual void SetSelectionModeToMultiple() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeMultiple); };
  virtual void SetSelectionModeToExtended() 
    { this->SetSelectionMode(vtkKWTkOptions::SelectionModeExtended); };

  // Description:
  // Set/Get the selection type.
  // Specifies one of two selection types for the tablelist widget: row or
  // cell. If the selection type is row then the default bindings will select
  // and deselect entire items, and the whole row having the location cursor
  // will be displayed as active when the tablelist has the keyboard focus.
  // If the selection type is cell then the default bindings will select and
  // deselect individual elements, and the single cell having the location 
  // cursor will be displayed as active when the tablelist has the keyboard 
  // focus. 
  //BTX
  enum 
  {
    SelectionTypeRow,
    SelectionTypeCell,
    SelectionTypeUnknown
  };
  //ETX
  virtual int GetSelectionType();
  virtual void SetSelectionType(int align);
  virtual void SetSelectionTypeToRow()
    { this->SetSelectionType(vtkKWMultiColumnList::SelectionTypeRow); };
  virtual void SetSelectionTypeToCell()
    { this->SetSelectionType(vtkKWMultiColumnList::SelectionTypeCell); };

  // Description:
  // Select/deselect a row, or single row (any other selection is cleared).
  virtual void SelectRow(int row_index);
  virtual void DeselectRow(int row_index);
  virtual void SelectSingleRow(int row_index);

  // Description:
  // Check if row is selected (i.e. any element in the row is selected)
  virtual int IsRowSelected(int row_index);

  // Description:
  // Select/deselect a cell, or single cell (any other selection is cleared).
  virtual void SelectCell(int row_index, int col_index);
  virtual void DeselectCell(int row_index, int col_index);
  virtual void SelectSingleCell(int row_index, int col_index);

  // Description:
  // Check if cell is selected
  virtual int IsCellSelected(int row_index, int col_index);

  // Description:
  // Get index of first selected row.
  // Returns -1 on error.
  virtual int GetIndexOfFirstSelectedRow();

  // Description:
  // Clear selection
  virtual void ClearSelection();

  // Description:
  // Specifies a command to be invoked when the selection has changed
  virtual void SetSelectionChangedCommand(
    vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when the interactive editing of a cell's
  // contents is started. The command is automatically concatenated with the
  // name of the tablelist widget (TableList), the cell's row and column
  // indices, as well as the text displayed in the cell, the resulting script
  // is evaluated in the global scope, and the return value becomes the 
  // initial contents of the temporary embedded widget used for the editing.
  virtual void SetEditStartCommand(vtkObject* object, const char *method);

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
  virtual void SetEditEndCommand(vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be invoked when mouse button 1 is pressed over one
  // of the header labels and later released over the same label. When the
  // <ButtonRelease-1> event occurs, the command is automatically 
  // concatenated with the name of the tablelist widget and the column index
  // of the respective label, and the resulting script is evaluated in the
  // global scope. 
  virtual void SetLabelCommand(vtkObject* object, const char *method);

  // Description:
  // Specifies a command to be used for the comparison of the items when
  // invoking the sort subcommand of the Tcl command associated with the
  // tablelist widget. To compare two items (viewed as lists of cell contents
  // within one row each) during the sort operation, the command is 
  // automatically concatenated with the two items and the resulting script
  // is evaluated. The script should return an integer less than, equal to, or
  // greater than zero if the first item is to be considered less than, equal
  // to, or greater than the second, respectively.
  virtual void SetSortCommand(vtkObject* object, const char *method);

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

  char *SelectionChangedCommand;

  // Description:
  // Set/Get a column configuration option (ex: "-bg") 
  virtual int SetColumnConfigurationOption(
    int col_index, const char* option, const char *value);
  virtual int HasColumnConfigurationOption(
    int col_index, const char* option);
  virtual const char* GetColumnConfigurationOption(
    int col_index, const char* option);
  virtual int GetColumnConfigurationOptionAsInt(
    int col_index, const char* option);
  virtual int SetColumnConfigurationOptionAsInt(
    int col_index, const char* option, int value);
  virtual void SetColumnConfigurationOptionAsText(
    int col_index, const char *option, const char *value);
  virtual const char* GetColumnConfigurationOptionAsText(
    int col_index, const char *option);

  // Description:
  // Set/Get a row configuration option (ex: "-bg") 
  virtual int SetRowConfigurationOption(
    int row_index, const char* option, const char *value);
  virtual int HasRowConfigurationOption(
    int row_index, const char* option);
  virtual const char* GetRowConfigurationOption(
    int row_index, const char* option);
  virtual int GetRowConfigurationOptionAsInt(
    int row_index, const char* option);
  virtual int SetRowConfigurationOptionAsInt(
    int row_index, const char* option, int value);

  // Description:
  // Set/Get a cell configuration option (ex: "-bg") 
  virtual int SetCellConfigurationOption(
    int row_index, int col_index, const char* option, const char *value);
  virtual int HasCellConfigurationOption(
    int row_index, int col_index, const char* option);
  virtual const char* GetCellConfigurationOption(
    int row_index, int col_index, const char* option);
  virtual int GetCellConfigurationOptionAsInt(
    int row_index, int col_index, const char* option);
  virtual int SetCellConfigurationOptionAsInt(
    int row_index, int col_index, const char* option, int value);
  virtual void SetCellConfigurationOptionAsText(
    int row_index, int col_index, const char *option, const char *value);
  virtual const char* GetCellConfigurationOptionAsText(
    int row_index, int col_index, const char *option);

private:
  vtkKWMultiColumnList(const vtkKWMultiColumnList&); // Not implemented
  void operator=(const vtkKWMultiColumnList&); // Not implemented
};

#endif
