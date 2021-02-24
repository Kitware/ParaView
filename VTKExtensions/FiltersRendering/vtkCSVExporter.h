/*=========================================================================

  Program:   ParaView
  Module:    vtkCSVExporter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCSVExporter
 * @brief   exporter used by certain views to export data as CSV.
 *
 * vtkCSVExporter can be used to generate comma-separated files. Unlike
 * `vtkCSVWriter`, this class can generate a single CSV from multiple array
 * collections. This avoids the need to generate of single large appended table.
 * The multiple arrays together represent the complete tabular
 * data that gets exported. The table can be split column-wise or row-wise
 * between multiple arrays. The `vtkCSVExporter` provides
 * two sets of APIs to handle the two cases. They cannot be mixed.
 *
 * When exporting array instances split by columns, i.e. each array
 * will have exactly same number of rows but different columns that are to be
 * concatenated together, use the `STREAM_COLUMNS` mode. To use this mode,
 * start writing by using `vtkCSVExporter::Open(vtkCSVExporter::STREAM_COLUMNS)`
 * and then add each column using `vtkCSVExporter::AddColumn`. Finally, complete
 * the export can generate the output using `vtkCSVExporter::Close`.
 *
 * When exporting arrays that are split by rows, use the STREAM_ROWS mode.
 * The arrays are provided as a part of vtkFieldData (or subclass). To begin
 * exporting in this mode, use
 * `vtkCSVExporter::Open(vtkCSVExporter::STREAM_ROWS)`. Write the header using
 * `vtkCSVExporter::WriteHeader(vtkFieldData*)` and the pass rows in order by
 * using `vtkCSVExporter::WriteData(vtkFieldData*) multiple times. Finally,
 * close using `vtkCSVExporter::Close`.
 *
 * In STREAM_ROWS mode, the exporter supports invalid / empty cells. When
 * writing each column in `WriteData` call, for each column-name an
 * vtkUnsignedCharArray with the name `__vtkValidMask__{COLUMN_NAME}` is looked
 * up. If found, it's value is used to determine if that cell is to written out
 * or not.
 */

#ifndef vtkCSVExporter_h
#define vtkCSVExporter_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

#include <string> // needed for std::string

class vtkAbstractArray;
class vtkDataArray;
class vtkFieldData;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkCSVExporter : public vtkObject
{
public:
  static vtkCSVExporter* New();
  vtkTypeMacro(vtkCSVExporter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set the delimiter use to separate fields ("," by default.)
   */
  vtkSetStringMacro(FieldDelimiter);
  vtkGetStringMacro(FieldDelimiter);
  //@}

  enum ExporterModes
  {
    STREAM_ROWS,
    STREAM_COLUMNS
  };

  //@{
  /**
   * Set a formatting to use when writing real numbers
   * (aka floating-point numbers) to csv.
   * See the std::fixed doc for more info.
   * Default is vtkVariant::DEFAULT_FORMATTING
   */
  vtkSetMacro(Formatting, int);
  vtkGetMacro(Formatting, int);
  //@}

  //@{
  /**
   * Set a precision to use when writing real numbers
   * (aka floating-point numbers) to csv.
   * See the std::setprecision doc for more info.
   * Default is 6
   */
  vtkSetMacro(Precision, int);
  vtkGetMacro(Precision, int);
  //@}

  //@{
  /**
   * In STREAM_ROWS mode, this API can be used to change columns labels
   * when exporting.
   */
  void SetColumnLabel(const char* name, const char* label);
  void ClearColumnLabels();
  const char* GetColumnLabel(const char* name);
  //@}

  /**
   * Open the file and set mode in which the exporter is operating.
   */
  bool Open(ExporterModes mode = STREAM_ROWS);

  /**
   * Closes the file cleanly. Call this at the end to close the file and dump
   * out any cached data.
   */
  void Close();

  /**
   * Same as Close except deletes the file, if created. This is useful to
   * interrupt the exporting on failure.
   */
  void Abort();

  //@{
  /**
   * In STREAM_ROWS mode, use these methods to write column headers once using
   * WriteHeader and then use WriteData as many times as needed to write out
   * rows.
   */
  void WriteHeader(vtkFieldData*);
  void WriteData(vtkFieldData*);
  //@}

  /**
   * In STREAM_COLUMNS mode, use this method to add a column (\c yarray). One
   * can assign it a name different the the name of the array using \c
   * yarrayname. If \c xarray is not nullptr, then is used as the row-id. This
   * makes it possible to add multiple columns with varying number of samples.
   * The final output will have empty cells for missing values.
   */
  void AddColumn(
    vtkAbstractArray* yarray, const char* yarrayname = nullptr, vtkDataArray* xarray = nullptr);

  //@{
  /**
   * Whether to output to a string instead of to a file, which is the default.
   */
  vtkSetMacro(WriteToOutputString, bool);
  vtkGetMacro(WriteToOutputString, bool);
  vtkBooleanMacro(WriteToOutputString, bool);
  //@}

  /**
   * Get the exported data as string.
   * If WriteToOutputString is OFF, returned string is empty.
   * If Close() was not called, returned string is empty.
   */
  std::string GetOutputString();

protected:
  vtkCSVExporter();
  ~vtkCSVExporter() override;

  char* FileName = nullptr;
  char* FieldDelimiter = nullptr;
  std::ostream* OutputStream = nullptr;
  ExporterModes Mode = STREAM_ROWS;

  bool WriteToOutputString = false;
  std::string OutputString;

  int Formatting;
  int Precision = 6;

private:
  vtkCSVExporter(const vtkCSVExporter&) = delete;
  void operator=(const vtkCSVExporter&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
