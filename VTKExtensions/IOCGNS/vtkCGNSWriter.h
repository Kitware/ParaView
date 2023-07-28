// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Maritime Research Institute Netherlands (MARIN)
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkCGNSWriter
 * @brief Writes CGNS files
 *
 * This writer writes (composite) datasets that may consist of
 *   - vtkImageData
 *   - vtkRectilinearGrid
 *   - vtkStructuredGrid
 *   - vtkUnstructuredGrid
 *   - vtkPolydata
 *   - vtkCompositeDataSet
 */

#ifndef vtkCGNSWriter_h
#define vtkCGNSWriter_h

#include "vtkPVVTKExtensionsIOCGNSWriterModule.h" // for export macro
#include "vtkWriter.h"

class vtkDoubleArray;

class VTKPVVTKEXTENSIONSIOCGNSWRITER_EXPORT vtkCGNSWriter : public vtkWriter
{
public:
  static vtkCGNSWriter* New();
  vtkTypeMacro(vtkCGNSWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get Name for the output file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  ///@}

  ///@{
  /**
   * When UseHDF5 is turned ON, the CGNS file will use HDF5 as
   * the underlying file format. When turned OFF, the file will use ADF as the
   * underlying file format.
   *
   * The Default is ON.
   */
  void SetUseHDF5(bool);
  vtkGetMacro(UseHDF5, bool);
  vtkBooleanMacro(UseHDF5, bool);
  ///@}

  ///@{
  /**
   * When WriteAllTimeSteps is turned ON, the writer is executed once for
   * each timestep available from the reader.
   *
   * The Default is OFF.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  vtkBooleanMacro(WriteAllTimeSteps, bool);
  ///@}

  ///@{
  /**
   * Sets the GhostLevel. Currently, this does
   * nothing, but this property is present
   * to ensure compatibility with ParaView.
   *
   * The Default is 0.
   */
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  ///@}

  ///@{
  /**
   * Provides an option to pad the time step when writing out time series data.
   * Only allow this format: ABC%.Xd where ABC is an arbitrary string which may
   * or may not exist and d must exist and d must be the last character
   * '.' and X may or may not exist, X must be an integer if it exists.
   * Default is nullptr.
   */
  vtkGetStringMacro(FileNameSuffix);
  vtkSetStringMacro(FileNameSuffix);
  ///@}

protected:
  vtkCGNSWriter();
  ~vtkCGNSWriter() override;

  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override; // pure virtual override from vtkWriter

  char* FileName = nullptr;
  bool UseHDF5 = true;
  bool WriteAllTimeSteps = false;
  char* FileNameSuffix = nullptr;

  int GhostLevel = 0;
  int NumberOfTimeSteps = 0;
  int CurrentTimeIndex = 0;
  vtkDoubleArray* TimeValues = nullptr;

  vtkDataObject* OriginalInput = nullptr;
  bool WasWritingSuccessful = false;

private:
  vtkCGNSWriter(const vtkCGNSWriter&) = delete;
  void operator=(const vtkCGNSWriter&) = delete;

  class vtkPrivate;
  friend class vtkPrivate;
};

#endif // vtkCGNSWriter_h
