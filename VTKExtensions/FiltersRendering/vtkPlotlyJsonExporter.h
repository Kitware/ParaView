// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlotlyJsonExporter
 * @brief   exporter used by certain views to export data into a file or stream.
 *
 */

#ifndef vtkPlotlyJsonExporter_h
#define vtkPlotlyJsonExporter_h

#include "vtkAbstractChartExporter.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

#include <string> // needed for std::string

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkPlotlyJsonExporter
  : public vtkAbstractChartExporter
{
public:
  static vtkPlotlyJsonExporter* New();
  vtkTypeMacro(vtkPlotlyJsonExporter, vtkAbstractChartExporter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  using vtkAbstractChartExporter::ExporterModes;
  /**
   * Open the file and set mode in which the exporter is operating.
   */
  bool Open(ExporterModes mode = STREAM_ROWS) override;

  /**
   * Closes the file cleanly. Call this at the end to close the file and dump
   * out any cached data.
   */
  void Close() override;

  /**
   * Same as Close except deletes the file, if created. This is useful to
   * interrupt the exporting on failure.
   */
  void Abort() override;

  ///@{
  /**
   * No Supported by vtkPlotlyJsonExporter
   */
  void WriteHeader(vtkFieldData*) override;
  void WriteData(vtkFieldData*) override;
  ///@}

  /**
   * Add a new plot using xarray as "x" values and yarray as "y" values.
   * The name of the plot will be yarrayname
   */
  void AddColumn(vtkAbstractArray* yarray, const char* yarrayname = nullptr,
    vtkDataArray* xarray = nullptr) override;

  /**
   * @brief Add the stringified \p attribute under then name \p attributeName in the Adios file.
   *
   */
  void AddStyle(vtkPlot* plot, const char* plotName) override;

  /**
   * @brief
   */
  void SetGlobalStyle(vtkChart* chart) override;

  ///@{
  /**
   * Whether to output to a string instead of to a file which is the default.
   */
  vtkSetMacro(WriteToOutputString, bool);
  vtkGetMacro(WriteToOutputString, bool);
  vtkBooleanMacro(WriteToOutputString, bool);
  ///@}

  // When WriteToOutputString is set this can be used to retrieve the string respresentation.
  std::string GetOutputString() const;

protected:
  vtkPlotlyJsonExporter();
  ~vtkPlotlyJsonExporter() override;

private:
  vtkPlotlyJsonExporter(const vtkPlotlyJsonExporter&) = delete;
  void operator=(const vtkPlotlyJsonExporter&) = delete;

  char* FileName = nullptr;
  class vtkInternals;
  bool WriteToOutputString = false;
  vtkInternals* Internals;
};

#endif
