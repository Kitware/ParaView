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
 * @brief   exporter used by certain views to export data as a CSV
 * file.
 *
 * This is used by vtkSMCSVExporterProxy to export the data shown in the
 * spreadsheet view or chart views as a CSV. The reason this class simply
 * doesn't use a vtkCSVWriter is that vtkCSVWriter is designed to write out a
 * single vtkTable as CSV. For exporting data from views, generating this single
 * vtkTable that can be exported is often time consuming or memory consuming or
 * both. Having a special exporter helps us with that. It provides two sets of
 * APIs:
 *
 * \li \c STREAM_ROWS: to use to stream a single large vtkTable as contiguous chunks where each
 * chuck
 * is a subset of the rows (ideal for use by vtkSpreadSheetView) viz. OpenFile,
 * WriteHeader, WriteData (which can be repeated as many times as needed), and CloseFile,
 * \li \c STREAM_COLUMNS: to use to add columns (idea for chart views) viz. OpenFile,
 * AddColumn (which can be repeated), and CloseFile.
 *
 * One has to pick which mode the exporter is operating in during the OpenFile()
 * call.
*/

#ifndef vtkCSVExporter_h
#define vtkCSVExporter_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkAbstractArray;
class vtkDataArray;
class vtkFieldData;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkCSVExporter : public vtkObject
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
   * yarrayname. If \c xarray is not NULL, then is used as the row-id. This
   * makes it possible to add multiple columns with varying number of samples.
   * The final output will have empty cells for missing values.
   */
  void AddColumn(
    vtkAbstractArray* yarray, const char* yarrayname = NULL, vtkDataArray* xarray = NULL);

protected:
  vtkCSVExporter();
  ~vtkCSVExporter() override;

  char* FileName;
  char* FieldDelimiter;
  ofstream* FileStream;
  ExporterModes Mode;

private:
  vtkCSVExporter(const vtkCSVExporter&) = delete;
  void operator=(const vtkCSVExporter&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
