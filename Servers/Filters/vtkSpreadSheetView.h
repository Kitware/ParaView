/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpreadSheetView
// .SECTION Description
// vtkSpreadSheetView is a vtkPVView subclass for a view used to show the
// any data as a spreadsheet. This view can only show one representation at a
// time. If more than one representation is added to this view, only the first
// visible representation will be shown.

#ifndef __vtkSpreadSheetView_h
#define __vtkSpreadSheetView_h

#include "vtkPVView.h"

class vtkClientServerMoveData;
class vtkCSVExporter;
class vtkMarkSelectedRows;
class vtkReductionFilter;
class vtkSortedTableStreamer;
class vtkTable;
class vtkVariant;

class VTK_EXPORT vtkSpreadSheetView : public vtkPVView
{
public:
  static vtkSpreadSheetView* New();
  vtkTypeMacro(vtkSpreadSheetView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Triggers a high-resolution render.
  // @CallOnAllProcessess
  virtual void StillRender() { this->Update(); }

  // Description:
  // Triggers a interactive render. Based on the settings on the view, this may
  // result in a low-resolution rendering or a simplified geometry rendering.
  // @CallOnAllProcessess
  virtual void InteractiveRender() { this->Update(); }

  // Description:
  // Overridden to identify and locate the active-representation.
  virtual void Update();

  // Description:
  // Get/Set if the view shows extracted selection only or the actual data.
  // false by default.
  // @CallOnAllProcessess
  void SetShowExtractedSelection(bool);
  vtkBooleanMacro(ShowExtractedSelection, bool);
  vtkGetMacro(ShowExtractedSelection, bool);

  // Description:
  // Get the number of columns.
  // @CallOnClient
  vtkIdType GetNumberOfColumns();

  // Description:
  // Get the number of rows.
  // @CallOnClient
  vtkIdType GetNumberOfRows();

  // Description:
  // Returns the name for the column.
  // @CallOnClient
  const char* GetColumnName(vtkIdType index);

  // Description:
  // Returns the value at given location. This may result in collective
  // operations is data is not available locally. This method can only be called
  // on the CLIENT process for now.
  // @CallOnClient
  vtkVariant GetValue(vtkIdType row, vtkIdType col);
  vtkVariant GetValueByName(vtkIdType row, const char* columnName);

  // Description:
  // Returns true if the row is selected.
  bool IsRowSelected(vtkIdType row);

  // Description:
  // Returns true is the data for the particular row is locally available.
  bool IsAvailable(vtkIdType row);

  //***************************************************************************
  // Forwarded to vtkSortedTableStreamer.
  // Description:
  // Get/Set the column name to sort by.
  // @CallOnAllProcessess
  void SetColumnNameToSort(const char*);
  void SetColumnNameToSort() { this->SetColumnNameToSort(NULL); }

  // Description:
  // Get/Set the component to sort with. Use -1 (default) for magnitude.
  // @CallOnAllProcessess
  void SetComponentToSort(int val);

  // Description:
  // Get/Set whether the sort order must be Max to Min rather than Min to Max.
  // @CallOnAllProcessess
  void SetInvertSortOrder(bool);

  // Description:
  // Set the block size
  // @CallOnAllProcessess
  void SetBlockSize(vtkIdType val);

  // Description:
  // Export the contents of this view using the exporter.
  bool Export(vtkCSVExporter* exporter);

//BTX
  // INTERNAL METHOD. Don't call directly.
  void FetchBlockCallback(vtkIdType blockindex);

protected:
  vtkSpreadSheetView();
  ~vtkSpreadSheetView();

  void OnRepresentationUpdated();

  vtkTable* FetchBlock(vtkIdType blockindex);

  void ClearCache();

  bool ShowExtractedSelection;
  vtkSortedTableStreamer* TableStreamer;
  vtkMarkSelectedRows* TableSelectionMarker;
  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;

  vtkIdType NumberOfRows;

  enum
    {
    FETCH_BLOCK_TAG = 394732
    };
private:
  vtkSpreadSheetView(const vtkSpreadSheetView&); // Not implemented
  void operator=(const vtkSpreadSheetView&); // Not implemented

  class vtkInternals;
  friend class vtkInternals;
  vtkInternals* Internals;

  bool SomethingUpdated;

  unsigned long RMICallbackTag;
//ETX
};

#endif
