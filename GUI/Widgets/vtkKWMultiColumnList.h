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
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWMultiColumnListWithScrollbars

#ifndef __vtkKWMultiColumnList_h
#define __vtkKWMultiColumnList_h

#include "vtkKWCoreWidget.h"

class vtkKWIcon;
class vtkKWMultiColumnListInternals;

class KWWidgets_EXPORT vtkKWMultiColumnList : public vtkKWCoreWidget
{
public:
  static vtkKWMultiColumnList* New();
  vtkTypeRevisionMacro(vtkKWMultiColumnList,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

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
  // Convenience method to Set the current background and
  // foreground color of the widget
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Insert a column just before the column given by col_index. If col_index
  // is equal to (or greater than) the number of columns the new column is
  // added to the end of the column list. The AddColumn method can be
  // used to add a column directly to the end of the list.
  // Returns the index of the column
  virtual int InsertColumn(int col_index, const char *title);
  virtual int AddColumn(const char *title);

  // Description:
  // Set a column name. Most of the API in this class uses numerical indices
  // to refer to columns. Yet, the index of a column can change if columns
  // are added or removed. Assigning a unique name to a column provides a
  // way to refer to a column without worrying about its location. Use the
  // GetColumnIndexWithName() to query the index of a column given its name.
  // indexing is done using numerical index.
  // Note that the name of a column has nothing to do with its title, which
  // is used to label the column in the table.
  virtual void SetColumnName(int col_index, const char *name);
  virtual const char* GetColumnName(int col_index);
  virtual int GetColumnIndexWithName(const char *name);

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
  // Specifies additional space to provide above and below each row of the
  // widget.
  virtual void SetRowSpacing(int);
  virtual int GetRowSpacing();

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
  // Set/Get the type of the temporary embedded widget to be used for
  // interactive editing of the contents of the given column's cells.
  // It can be one of entry (which is the default), spinbox or checkbutton
  // at the moment. 
  // This can be set at the cell level too (see SetCellEditWindow).
  // Note that this setting controls the widget used for *editing*, not for
  // display. The cell contents is still displayed using whatever text (see
  // SetCellText), image (see SetCellImage) or custom window (see 
  // SetCellWindowCommand) is defined at the cell level. Check the
  // SetCellWindowCommandToCheckButton or SetCellWindowCommandToColorButton
  // methods for more advanced display *and* editing features.
  //BTX
  enum 
  {
    ColumnEditWindowEntry = 0,
    ColumnEditWindowCheckButton,
    ColumnEditWindowSpinBox,
    ColumnEditWindowUnknown
  };
  //ETX
  virtual int GetColumnEditWindow(int col_index);
  virtual void SetColumnEditWindow(int col_index, int arg);
  virtual void SetColumnEditWindowToEntry(int col_index)
    { this->SetColumnEditWindow(
      col_index, vtkKWMultiColumnList::ColumnEditWindowEntry); };
  virtual void SetColumnEditWindowToCheckButton(int col_index)
    { this->SetColumnEditWindow(
      col_index, vtkKWMultiColumnList::ColumnEditWindowCheckButton); };
  virtual void SetColumnEditWindowToSpinBox(int col_index)
    { this->SetColumnEditWindow(
      col_index, vtkKWMultiColumnList::ColumnEditWindowSpinBox); };

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
  // Specifies the command to be invoked when displaying the contents of a
  // cell within a column col_index or adding them to the selection when the
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
  // Also check SetColumnFormatCommandToEmptyOutput, which can be used
  // to set the ColumnFormatCommand to return an empty output.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the current cell's text for that col_index column: const char*
  // The following output is expected from the command:
  // - the text to be displayed in the cell instead: const char*
  virtual void SetColumnFormatCommand(int col_index, 
                                      vtkObject *object, const char *method);
  // Description:
  // Specifies the command to be invoked when displaying the contents of a
  // cell within a column col_index.
  // This is a convenience method to set the ColumnFormatCommand to return an
  // empty output. This comes in handy if only images or embedded windows are
  // to be displayed in a column but the texts associated with the cells may
  // not simply be empty strings because they are needed for other purposes
  // (like sorting or editing). In such cases, a command returning an empty
  // string can be used, thus making sure that the textual information 
  // contained in that column remains hidden. This method can be used just
  // for that instead of SetColumnFormatCommand.
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
  // found, its row index is used to set the contents of the cell, if it is
  // not found, a new row is inserted.
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
  // Get the current cell background or foreground color
  // In order of priority:
  // - if not selected, color is: cell > row > stripe > column > widget.
  // - if selected, color is: cell > row > column > widget.
  virtual void GetCellCurrentBackgroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellCurrentBackgroundColor(int row_index, int col_index);
  virtual void GetCellCurrentForegroundColor(
    int row_index, int col_index, double *r, double *g, double *b);
  virtual double* GetCellCurrentForegroundColor(int row_index, int col_index);

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
  // color (as returned by GetCellBackgroundColor). But since sorting
  // a column, or inserting new rows, can change the position of the cell
  // in a stripe (see SetStripeBackgroundColor), it is best to:
  //   - use images that do not have an alpha component, or 
  //   - refresh the image periodically (each time a row is added/removed)
  //     also check SetPotentialCellColorsChangedCommand and
  //     RefreshAllCellWindowCommands.
  virtual void SetCellImage(int row_index, int col_index, const char *);
  virtual void SetCellImageToIcon(
    int row_index, int col_index, vtkKWIcon *icon);
  virtual void SetCellImageToPredefinedIcon(
    int row_index, int col_index, int icon_index);
  virtual void SetCellImageToPixels(
    int row_index, int col_index,
    const unsigned char *pixels, int width, int height, int pixel_size,
    unsigned long buffer_length = 0);
  virtual const char* GetCellImage(int row_index, int col_index);

  // Description:
  // Set/Get the type of the temporary embedded widget to be used for
  // interactive editing of the contents of the cell.
  // This option overrides the one with the same name for the column
  // containing the given cell, and may have the same values as its
  // column-related counterpart (see SetColumnEditWindow).
  // It can be one of entry (which is the default), spinbox or checkbutton
  // at the moment.
  // Note that this setting controls the widget used for *editing*, not for
  // display. The cell contents is still displayed using whatever text (see
  // SetCellText), image (see SetCellImage) or custom window (see 
  // SetCellWindowCommand) is defined at the cell level. Check the
  // SetCellWindowCommandToCheckButton or SetCellWindowCommandToColorButton
  // methods for more advanced display *and* editing features.
  //BTX
  enum 
  {
    CellEditWindowEntry = 0,
    CellEditWindowCheckButton,
    CellEditWindowSpinBox,
    CellEditWindowUnknown
  };
  //ETX
  virtual int GetCellEditWindow(int row_index, int col_index);
  virtual void SetCellEditWindow(int row_index, int col_index, int arg);
  virtual void SetCellEditWindowToEntry(int row_index, int col_index)
    { this->SetCellEditWindow(
      row_index, col_index, vtkKWMultiColumnList::CellEditWindowEntry); };
  virtual void SetCellEditWindowToCheckButton(int row_index, int col_index)
    { this->SetCellEditWindow(
      row_index,col_index, vtkKWMultiColumnList::CellEditWindowCheckButton);};
  virtual void SetCellEditWindowToSpinBox(int row_index, int col_index)
    { this->SetCellEditWindow(
      row_index, col_index, vtkKWMultiColumnList::CellEditWindowSpinBox); };

  // Description:
  // Specifies a command to create the window (i.e. widget) to be embedded
  // into the cell located at (row_index, col_index).
  // The command is automatically concatenated with the name of the tablelist
  // widget, the cell's row and column indices, as well as the path name of
  // the embedded window to be created, and the resulting script is evaluated
  // in the global scope. This path name can be used to create your own
  // vtkKWWidget by assigning the widget's name manually using vtkKWWidget's
  // SetWidgetName method.
  // In most case, you should attempt to set the widget's background and
  // foreground colors to match the cell's background and foreground colors
  // (which can be retrieved using GetCellCurrentBackgroundColor and 
  // GetCellCurrentForegroundColor). 
  // Since the background and foreground colors of the cell change dynamically
  // depending on the sorting order and the selected rows, you should set
  // the SetPotentialCellColorsChangedCommand to this object's own
  // RefreshColorsOfAllCellsWithWindowCommand method so that each time
  // the cell colors change, this user-defined widget is refreshed.
  // Also, if you have set a text contents in the same cell (using SetCellText)
  // you may want to hide it automatically using 
  // SetColumnFormatCommandToEmptyOutput.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the name of the internal tablelist widget (to be ignored): const char*
  // - the cell location, i.e. its row and column indices: int, int
  // - the path name of the embedded window/widget to be created: const char*
  virtual void SetCellWindowCommand(
    int row_index, int col_index, vtkObject *object, const char *method);

  // Description:
  // The SetCellWindowCommandToCheckButton method is a very convenient
  // way to automatically display a checkbutton in the cell. The selected
  // state of the button is interpreted directly from the text in the cell
  // (as set by SetCellText for example), and thus should be either 0 or 1.
  // The editable flag of the cell is automatically set to 0, do
  // not change it manually using SetCellEditable. 
  // When the checkbutton selected state changes, the contents of
  // the cell is updated automatically (to 0 or 1). Note that the
  // EditEndCommand and CellUpdatedCommand are handled the same way.
  // Check the SetCellWindowCommand method for more information.
  virtual void SetCellWindowCommandToCheckButton(int row_index, int col_index);

  // Description:
  // The SetCellWindowCommandToReadOnlyComboBox is a convenience method to 
  // add a list of items in a combo box within one of the cells. For instance, 
  // similar items may be grouped and added in a pull down list. This also
  // prevents overcrowding the Multi-column list by having things hidden in a 
  // combo box.
  virtual void SetCellWindowCommandToReadOnlyComboBox(int row_index, int col_index);
  virtual void SetNthEntryInReadOnlyComboBox(int i, const char *value, 
                                                      int row, int col);
  virtual void DeleteNthEntryInReadOnlyComboBox(int i, int row, int col);
  virtual void DeleteAllEntriesInReadOnlyComboBox(int row, int col);

  // Description:
  // The SetCellWindowCommandToColorButton method is a convenient
  // way to automatically display a color button in the cell. The color of
  // the button is interpreted directly from the text in the cell 
  // (as set by SetCellText for example), provided it is a space separated
  // list of 3 normalized floating point numbers representing the
  // red, green and blue components of the color (ex: "1.0 0.2 0.6").
  // When the color button is edited (if the column or cell is
  // made editable), a color dialog pops up so that the user can pick
  // a new color. The contents of the cell is updated automatically 
  // with the new color value, as a similar space separated list of
  // normalized R, G, B values. Note that the EditStartCommand, 
  // EditEndCommand and CellUpdatedCommand are handled the same way.
  // Check the SetCellWindowCommand method for more information.
  virtual void SetCellWindowCommandToColorButton(int row_index, int col_index);

  // Description:
  // Specifies a command to be invoked when the window embedded into the cell
  // located at (row_index, col_index) is destroyed. It is automatically 
  // concatenated the same parameter as the SetCellWindowCommand method that
  // was used to create the embedded window.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the name of the internal tablelist widget (to be ignored): const char*
  // - the cell location, i.e. row and column indices: int, int
  // - the path name of the embedded window/widget to be created: const char*
  virtual void SetCellWindowDestroyCommand(
    int row_index, int col_index, vtkObject *object, const char *method);

  // Description:
  // The SetCellWindowDestroyCommandToRemoveChild method is a convenient
  // way to automatically set the CellWindowDestroyCommand to a callback that
  // will remove the child widget that matches the name of the Tk widget about
  // to be destroyed. This is very useful if the SetCellWindowCommand
  // is set to a callback that actually allocates a new vtkKWWidget object.
  // That way, each time the cell is about to be destroyed, it is
  // cleanly de-allocated first (by setting its Parent to NULL).
  virtual void SetCellWindowDestroyCommandToRemoveChild(
    int row_index, int col_index);

  // Description:
  // Force a cell (or all cells) for which a WindowCommand has been defined
  // to recreate its dynamic content. It does so by setting the WindowCommand
  // to NULL, than setting it to its previous value (per author's suggestion).
  virtual void RefreshCellWithWindowCommand(int row_index, int col_index);
  virtual void RefreshAllCellsWithWindowCommand();

  // Description:
  // Force a cell (or all cells) for which a WindowCommand has been defined
  // to set the background and foreground colors to the cell current
  // background and foreground colors.
  // It does so by tyring to safe-down-cast the widget inside that cell into
  // a vtkKWCoreWidget and set its background color to the color returned
  // by GetCellCurrentBackgroundColor and its foreground color to the color
  // returned by GetCellCurrentForegroundColor. It then performs the same
  // for the first level children of the widget inside that cell.
  // This can be useful when the cell contents is an image with an alpha
  // channel (transparency), or a user-defined dynamic widget 
  // (see SetCellWindowCommand)
  virtual void RefreshColorsOfCellWithWindowCommand(
    int row_index, int col_index);
  virtual void RefreshColorsOfAllCellsWithWindowCommand();

  // Description:
  // Retrieve the path of the window contained in the cell as created by 
  // the WindowCommand.
  virtual const char* GetCellWindowWidgetName(int row_index, int col_index);

  // Description:
  // Once a user-defined dynamic widget is created by the WindowCommand, 
  // clicking on it is likely *not* to trigger the same interactive behavior
  // as clicking on a regular cell (i.e., clicking on the widget will not
  // select the row or cell for example). This can be a good thing if
  // clicking on the widget is meant to be intercepted by the widget to
  // trigger a different behaviour, but in many other cases, one would want
  // the interaction bindings to remain the same and consistent for all rows.
  // In order to do so, the common widget row bindings have to be added to the
  // widget the was just created. To do so, call AddBindingsToWidget, either on
  // the Tk widget name, or on avtkKWWidget that may have been used to wrap
  // around that Tk widget name.
  // A complex widget can be made of several other sub-widgets that need
  // the bindings to be passed on too. Use AddBindingsToWidgetAndChildren
  // to pass the bindings to a widget and its chilren automatically (or
  // call AddBindingsToWidget manually on each sub-widgets).
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
  virtual int FindCellTextAsIntInColumn(int col_index, int value);

  // Description:
  // Edit cell (or cancel edit). If supported, edit cell contents interactively
  virtual void EditCell(int row_index, int col_index);
  virtual void CancelEditing();

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
  // Get the number of selected rows, and retrieve their indices (it is up
  // to the caller to provide a large enough buffer). Both returns the
  // number of selected rows.
  virtual int GetNumberOfSelectedRows();
  virtual int GetSelectedRows(int *indices);

  // Description:
  // Get index of first selected row.
  // Returns -1 on error.
  virtual int GetIndexOfFirstSelectedRow();

  // Description:
  // Select/deselect a cell, or single cell (any other selection is cleared).
  virtual void SelectCell(int row_index, int col_index);
  virtual void DeselectCell(int row_index, int col_index);
  virtual void SelectSingleCell(int row_index, int col_index);

  // Description:
  // Check if cell is selected
  virtual int IsCellSelected(int row_index, int col_index);

  // Description:
  // Get the number of selected cells, and retrieve their indices (it is up
  // to the caller to provide large enough buffers). Both returns the
  // number of selected cells.
  virtual int GetNumberOfSelectedCells();
  virtual int GetSelectedCells(int *row_indices, int *col_indices);

  // Description:
  // Clear selection
  virtual void ClearSelection();

  // Description:
  // Specifies whether or not a selection in the widget should also be the X
  // selection. If the selection is exported, then selecting in the widget
  // deselects the current X selection, selecting outside the widget deselects
  // any widget selection, and the widget will respond to selection retrieval
  // requests when it has a selection.  
  virtual void SetExportSelection(int);
  virtual int GetExportSelection();
  vtkBooleanMacro(ExportSelection, int);
  
  // Description:
  // Specifies a command to be invoked when an element is selected/deselected
  // in the widget. Re-selecting an element will trigger this command too.
  // If one want to be notified only when the selection has *changed* (the
  // number of selected/deselected items has changed), use the
  // SelectionChangedCommand command instead.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetSelectionCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when the selection has *changed*. This
  // command will *not* be invoked when an item is re-selected (i.e. it
  // was already selected when the user clicked on it again). To be notified
  // when any selection event occurs, use SelectionCommand instead.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetSelectionChangedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when any change is made that
  // can potentially affect the background color of a cell (selecting
  // a cell, sorting a column, adding/removing rows, etc). 
  // This is useful if a user-defined dynamic widget created in a cell
  // (using the SetCellWindowCommand methods)
  // is setting its own background color to match the background color
  // of a cell (using GetCellCurrentBackgroundColor). In that case,
  // just set this command to RefreshColorsOfAllCellsWithWindowCommand. 
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetPotentialCellColorsChangedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when a column has been sorted. 
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetColumnSortedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when the interactive editing of a cell's
  // contents is started. The command is automatically concatenated with
  // the cell's row and column indices, as well as the text displayed in
  // the cell, the resulting script is evaluated in the global scope, and
  // the return value becomes the initial contents of the temporary
  // embedded widget used for the editing.
  // The next step (validation) is handled by SetEditEndCommand (if any)
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the cell location, i.e. its row and column indices: int, int
  // - the current cell's text: const char*
  // The following output is expected from the command:
  // - the initial contents of the widget used for editing: const char*
  virtual void SetEditStartCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked on normal termination of the 
  // interactive editing of a cell's contents if the final text of the
  // temporary embedded widget used for the editing is different from its 
  // initial one. The command is automatically concatenated with the 
  // cell's row and column indices, as well as the final contents of the edit
  // window, the resulting script is evaluated in the global scope, and the
  // return value becomes the cell's new contents after destroying the
  // temporary embedded widget. The main purpose of this script is to perform
  // a final validation of the edit window's contents and eventually reject
  // the input by calling the RejectInput() method. Another purpose of this
  // command is to convert the edit window's text to the cell's new internal
  // contents, which is necessary if, due to the SetColumnFormatCommand option
  // the cell's internal value is different from its external representation. 
  // The next step (updating) is handled by SetCellUpdatedCommand (if any)
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the cell location, i.e. its row and column indices: int, int
  // - the final contents of the edit window: const char*
  // The following output is expected from the command:
  // - the cell's new contents: const char*
  virtual void SetEditEndCommand(vtkObject *object, const char *method);

  // Description:
  // If invoked from within EditEndCommand, this method prevents the
  // termination of the interactive editing of the contents of a cell.  It
  // enables you to reject the widget's text during the final validation of the
  // string intended to become the new cell contents.
  virtual void RejectInput();

  // Description:
  // Specifies a command to be invoked when a cell contents has been
  // successfully updated after editing it. The command is automatically
  // concatenated with the cell's row and column indices, as well as the
  // new contents of the cell. The main purpose of this script is to let
  // external/third-party applications/objects retrieve the new cell contents
  // and update their own internal values.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the cell location, i.e. its row and column indices: int, int
  // - the cell's new contents: const char*
  virtual void SetCellUpdatedCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when mouse button 1 is pressed over one
  // of the header labels and later released over the same label. When the
  // <ButtonRelease-1> event occurs, the command is automatically 
  // concatenated with the name of the tablelist widget and the column index
  // of the respective label, and the resulting script is evaluated in the
  // global scope. 
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the name of the internal tablelist widget (to be ignored): const char*
  // - the column index of the label: int
  virtual void SetLabelCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be used for the comparison of the items when
  // invoking the sort subcommand of the Tcl command associated with the
  // tablelist widget. To compare two items (viewed as lists of cell contents
  // within one row each) during the sort operation, the command is 
  // automatically concatenated with the two items and the resulting script
  // is evaluated. The script should return an integer less than, equal to, or
  // greater than zero if the first item is to be considered less than, equal
  // to, or greater than the second, respectively.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the contents of the first item/cell to compare: const char*
  // - the contents of the second item/cell to compare: const char*
  // The following output is expected from the command:
  // - the result of the comparison: int
  virtual void SetSortCommand(vtkObject *object, const char *method);

  // Description:
  // Specifies a command to be invoked when the user right-click on a cell.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the cell location, i.e. its row and column indices: int, int
  // - the pointer (x, y) absolute location the click occured at: int, int
  virtual void SetRightClickCommand(vtkObject *object, const char *method);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  // Description:
  // Callbacks. Internal, do not use.
  virtual void SelectionCallback();
  virtual void CellWindowDestroyRemoveChildCallback(
    const char*, int, int, const char*);
  virtual void CellUpdatedCallback();
  virtual const char* EditStartCallback(
    const char *widget, int row, int col, const char *text);
  virtual const char* EditEndCallback(
    const char *widget, int row, int col, const char *text);
  virtual void CellWindowCommandToCheckButtonCallback(
    const char*, int, int, const char*);
  virtual void CellWindowCommandToReadOnlyComboBoxCallback(
    const char*, int, int, const char*);
  virtual void CellWindowCommandToCheckButtonSelectCallback(
    vtkKWWidget*, int, int, int);
  virtual void CellWindowCommandToColorButtonCallback(
    const char*, int, int, const char*);
  virtual void ColumnSortedCallback();
  virtual void EnterCallback();
  virtual void RightClickCallback(
    const char *w, int x, int y, int root_x, int root_y);

protected:
  vtkKWMultiColumnList();
  ~vtkKWMultiColumnList();

  char *EditStartCommand;
  const char* InvokeEditStartCommand(int row, int col, const char *text);

  char *EditEndCommand;
  const char* InvokeEditEndCommand(int row, int col, const char *text);

  char *CellUpdatedCommand;
  void InvokeCellUpdatedCommand(int row, int col, const char *text);

  char *SelectionCommand;
  virtual void InvokeSelectionCommand();

  char *SelectionChangedCommand;
  virtual void InvokeSelectionChangedCommand();

  char *PotentialCellColorsChangedCommand;
  virtual void InvokePotentialCellColorsChangedCommand();

  char *ColumnSortedCommand;
  void InvokeColumnSortedCommand();

  char *RightClickCommand;
  void InvokeRightClickCommand(int row, int col, int x, int y);

  // Description:
  // Called when the number of rows/columns changed
  virtual void NumberOfRowsChanged();
  virtual void NumberOfColumnsChanged();

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

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWMultiColumnListInternals *Internals;
  //ETX

  // Description:
  // Check if the selection has changed and invoke the corresponding command
  virtual void HasSelectionChanged();

  // Description:
  // Find cell at relative coordinate x, y
  virtual int FindCellAtRelativeCoordinates(
    int x, int y, int *row_index, int *col_index);

private:
  vtkKWMultiColumnList(const vtkKWMultiColumnList&); // Not implemented
  void operator=(const vtkKWMultiColumnList&); // Not implemented
};

#endif
