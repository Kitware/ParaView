// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractChartExporter
 * @brief   exporter used by certain views to export data into a file or stream.
 *
 */

#ifndef vtkAbstractChartExporter_h
#define vtkAbstractChartExporter_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

#include <string> // needed for std::string

class vtkAbstractArray;
class vtkDataArray;
class vtkFieldData;
class vtkPlot;
class vtkChart;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkAbstractChartExporter : public vtkObject
{
public:
  static vtkAbstractChartExporter* New();
  vtkTypeMacro(vtkAbstractChartExporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ExporterModes
  {
    STREAM_ROWS,
    STREAM_COLUMNS
  };

  /**
   * Open the file and set mode in which the exporter is operating.
   */
  virtual bool Open(ExporterModes mode = STREAM_ROWS) = 0;

  /**
   * Closes the file cleanly. Call this at the end to close the file and dump
   * out any cached data.
   */
  virtual void Close() = 0;

  /**
   * Same as Close except deletes the file, if created. This is useful to
   * interrupt the exporting on failure.
   */
  virtual void Abort() = 0;

  ///@{
  /**
   * In STREAM_ROWS mode, use these methods to write column headers once using
   * WriteHeader and then use WriteData as many times as needed to write out
   * rows.
   */
  virtual void WriteHeader(vtkFieldData*) = 0;
  virtual void WriteData(vtkFieldData*) = 0;
  ///@}

  /**
   * In STREAM_COLUMNS mode, use this method to add a column (\c yarray). One
   * can assign it a name different the the name of the array using \c
   * yarrayname. If \c xarray is not nullptr, then is used as the row-id. This
   * makes it possible to add multiple columns with varying number of samples.
   * The final output will have empty cells for missing values.
   */
  virtual void AddColumn(
    vtkAbstractArray* yarray, const char* yarrayname = nullptr, vtkDataArray* xarray = nullptr) = 0;

  /**
   * Attach information about the style of the plot (color,line type, marker
   * type etc. To the plot accosiated with plotName. plotName should match the
   * yarrayname used when adding the data via Add Column.
   * AddColumn needs to be called before adding the style.
   */
  virtual void AddStyle(vtkPlot* plot, const char* plotName) = 0;

  /** Set Global style elements like graph and axis titles. Each implementation
   * is expected to get what it can support from the public vtkChart API.
   */
  virtual void SetGlobalStyle(vtkChart* chart) = 0;

protected:
  vtkAbstractChartExporter();
  ~vtkAbstractChartExporter() override;

private:
  vtkAbstractChartExporter(const vtkAbstractChartExporter&) = delete;
  void operator=(const vtkAbstractChartExporter&) = delete;
};

#endif
