/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCGNSWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
Copyright (c) Maritime Research Institute Netherlands (MARIN)
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class vtkCGNSWriter
 * @brief Writes CGNS files
 *
 * This writer writes (composite) datasets that may consist of
 *   - vtkStructuredGrid
 *   - vtkUnstructuredGrid
 *   - vtkPolydata
 *   - vtkCompositeDataSet
 */

#ifndef vtkCGNSWriter_h
#define vtkCGNSWriter_h

#include "vtkPVVTKExtensionsIOCGNSWriterModule.h" // for export macro
#include "vtkWriter.h"

class VTKPVVTKEXTENSIONSIOCGNSWRITER_EXPORT vtkCGNSWriter : public vtkWriter
{
public:
  static vtkCGNSWriter* New();
  vtkTypeMacro(vtkCGNSWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Name for the output file.  If writing in parallel, the number
   * of processes and the process rank will be appended to the name,
   * so each process is writing out a separate file.
   * If not set, this class will make up a file name.
   */

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  vtkBooleanMacro(UseHDF5, bool);
  void SetUseHDF5(bool);

  /**
   * When WriteAllTimeSteps is turned ON, the writer is executed once for
   * each timestep available from the reader.
   */
  vtkSetMacro(WriteAllTimeSteps, bool);
  vtkGetMacro(WriteAllTimeSteps, bool);
  vtkBooleanMacro(WriteAllTimeSteps, bool);

protected:
  bool WasWritingSuccessful = false;
  char* FileName = nullptr;
  vtkDataObject* OriginalInput = nullptr;
  bool UseHDF5 = true;

  bool WriteAllTimeSteps = false;
  class vtkDoubleArray* TimeValues = nullptr;
  int NumberOfTimeSteps = 0;
  int CurrentTimeIndex = 0;

  vtkCGNSWriter();
  ~vtkCGNSWriter() override;

  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override; // pure virtual override from vtkWriter

private:
  vtkCGNSWriter(const vtkCGNSWriter&) = delete;
  void operator=(const vtkCGNSWriter&) = delete;

  class vtkPrivate;
  friend class vtkPrivate;
};

#endif // vtkCGNSWriter_h
