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
 *   - vtkMultiBlockDataSet
 *   - vtkMultiPieceDataSet (currently not implemented)
*/

#ifndef vtkCGNSWriter_h
#define vtkCGNSWriter_h

#include "vtkPVVTKExtensionsCGNSWriterModule.h" // for export macro
#include "vtkWriter.h"

class VTKPVVTKEXTENSIONSCGNSWRITER_EXPORT vtkCGNSWriter : public vtkWriter
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

protected:
  vtkCGNSWriter();
  ~vtkCGNSWriter() override;

  char* FileName;
  vtkDataObject* OriginalInput;
  bool UseHDF5; //

  int ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  bool WasWritingSuccessful;
  void WriteData() override; // pure virtual override from vtkWriter

private:
  vtkCGNSWriter(const vtkCGNSWriter&) = delete;
  void operator=(const vtkCGNSWriter&) = delete;

  class vtkPrivate;
  friend class vtkPrivate;
};

#endif // vtkCGNSWriter_h
