/*=========================================================================

  Program:   ParaView
  Module:    vtkSpreadSheetView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpreadSheetView
 *
 * vtkSpreadSheetView is a vtkPVView subclass for a view used to show any data
 * as a spreadsheet. This view can only show one representation at a
 * time. If more than one representation is added to this view, only the first
 * visible representation will be shown.
*/

#ifndef vtkSpreadSheetView_h
#define vtkSpreadSheetView_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVView.h"

#include <map> // For Column Visibilities

class vtkCSVExporter;
class vtkClientServerMoveData;
class vtkMarkSelectedRows;
class vtkPassArrays;
class vtkReductionFilter;
class vtkSortedTableStreamer;
class vtkTable;
class vtkVariant;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkSpreadSheetView : public vtkPVView
{
public:
  static vtkSpreadSheetView* New();
  vtkTypeMacro(vtkSpreadSheetView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Triggers a high-resolution render.
   * \note CallOnAllProcesses
   */
  virtual void StillRender() VTK_OVERRIDE { this->StreamToClient(); }

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   * \note CallOnAllProcesses
   */
  virtual void InteractiveRender() VTK_OVERRIDE { this->StreamToClient(); }

  /**
   * Overridden to identify and locate the active-representation.
   */
  virtual void Update() VTK_OVERRIDE;

  //@{
  /**
   * Get/Set if the view shows extracted selection only or the actual data.
   * false by default.
   * \note CallOnAllProcesses
   */
  void SetShowExtractedSelection(bool);
  vtkBooleanMacro(ShowExtractedSelection, bool);
  vtkGetMacro(ShowExtractedSelection, bool);
  //@}

  //@{
  /**
   * Allow user to enable/disable cell connectivity generation in the datamodel
   */
  vtkSetMacro(GenerateCellConnectivity, bool);
  vtkGetMacro(GenerateCellConnectivity, bool);
  vtkBooleanMacro(GenerateCellConnectivity, bool);
  //@}

  //@{
  /**
   * Manage column visibilities, used only for export
   */
  void SetColumnVisibility(int fieldAssociation, const char* column, int visibility);
  void ClearColumnVisibilities();
  //@}

  /**
   * Get the number of columns.
   * \note CallOnClient
   */
  vtkIdType GetNumberOfColumns();

  /**
   * Get the number of rows.
   * \note CallOnClient
   */
  vtkIdType GetNumberOfRows();

  /**
   * Returns the name for the column.
   * \note CallOnClient
   */
  const char* GetColumnName(vtkIdType index);

  //@{
  /**
   * Returns the value at given location. This may result in collective
   * operations is data is not available locally. This method can only be called
   * on the CLIENT process for now.
   * \note CallOnClient
   */
  vtkVariant GetValue(vtkIdType row, vtkIdType col);
  vtkVariant GetValueByName(vtkIdType row, const char* columnName);
  //@}

  /**
   * Returns true if the row is selected.
   */
  bool IsRowSelected(vtkIdType row);

  /**
   * Returns true is the data for the particular row is locally available.
   */
  bool IsAvailable(vtkIdType row);

  //***************************************************************************
  // Forwarded to vtkSortedTableStreamer.
  /**
   * Get/Set the column name to sort by.
   * \note CallOnAllProcesses
   */
  void SetColumnNameToSort(const char*);
  void SetColumnNameToSort() { this->SetColumnNameToSort(NULL); }

  /**
   * Get/Set the component to sort with. Use -1 (default) for magnitude.
   * \note CallOnAllProcesses
   */
  void SetComponentToSort(int val);

  /**
   * Get/Set whether the sort order must be Max to Min rather than Min to Max.
   * \note CallOnAllProcesses
   */
  void SetInvertSortOrder(bool);

  /**
   * Set the block size
   * \note CallOnAllProcesses
   */
  void SetBlockSize(vtkIdType val);

  /**
   * Export the contents of this view using the exporter.
   */
  bool Export(vtkCSVExporter* exporter);

  /**
   * Allow user to clear the cache if he needs to.
   */
  void ClearCache();

  // INTERNAL METHOD. Don't call directly.
  vtkTable* FetchBlockCallback(vtkIdType blockindex, bool filterColumnForExport = false);

protected:
  vtkSpreadSheetView();
  ~vtkSpreadSheetView();

  /**
   * On render streams all the data from the processes to the client.
   * Returns 0 on failure.
   * Note: Was removed from update because you can't call update()
   * while in an update
   */
  int StreamToClient();

  void OnRepresentationUpdated();

  vtkTable* FetchBlock(vtkIdType blockindex, bool filterColumnForExport = false);

  bool ShowExtractedSelection;
  bool GenerateCellConnectivity;
  vtkSortedTableStreamer* TableStreamer;
  vtkMarkSelectedRows* TableSelectionMarker;
  vtkReductionFilter* ReductionFilter;
  vtkClientServerMoveData* DeliveryFilter;
  vtkPassArrays* PassFilter;

  vtkIdType NumberOfRows;

  enum
  {
    FETCH_BLOCK_TAG = 394732
  };

private:
  vtkSpreadSheetView(const vtkSpreadSheetView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSpreadSheetView&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  friend class vtkInternals;
  vtkInternals* Internals;

  std::map<std::pair<int, std::string>, int> ColumnVisibilities;
  bool SomethingUpdated;

  unsigned long RMICallbackTag;
};

#endif
