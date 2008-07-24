/*=========================================================================

  Program:   ParaView
  Module:    vtkTimeSeriesWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTimeSeriesWriter - parallel meta-writer for serial formats
// .SECTION Description:
// vtkTimeSeriesWriter is a meta-writer that enables serial writers
// to work in parallel. It gathers data to the 1st node and invokes the
// internal writer. Currently, only polydata is supported.

#ifndef __vtkTimeSeriesWriter_h
#define __vtkTimeSeriesWriter_h

#include "vtkDataObjectAlgorithm.h"

class vtkCompositeDataSet;
class vtkPolyData;
class vtkRectilinearGrid;

class VTK_EXPORT vtkTimeSeriesWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkTimeSeriesWriter* New();
  vtkTypeRevisionMacro(vtkTimeSeriesWriter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the internal writer.
  void SetWriter(vtkAlgorithm*);
  vtkGetObjectMacro(Writer, vtkAlgorithm);

  // Description:
  // Return the MTime also considering the internal writer.
  virtual unsigned long GetMTime();

  // Description:
  // Name of the method used to set the file name of the internal
  // writer. By default, this is SetFileName.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

  // Description:
  // Get/Set the name of the output file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Invoke the writer.  Returns 1 for success, 0 for failure.
  int Write();

  // Description:
  vtkGetMacro(WriteAllTimeSteps, int);
  vtkSetMacro(WriteAllTimeSteps, int);
  vtkBooleanMacro(WriteAllTimeSteps, int);
  
protected:
  vtkTimeSeriesWriter();
  ~vtkTimeSeriesWriter();

  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector);
  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);

private:
  vtkTimeSeriesWriter(const vtkTimeSeriesWriter&); // Not implemented.
  void operator=(const vtkTimeSeriesWriter&); // Not implemented.

  vtkRectilinearGrid* AppendBlocks(vtkCompositeDataSet* cds);

  void WriteAFile(const char* fname, vtkDataObject* input);

  void SetWriterFileName(const char* fname);

  vtkAlgorithm* Writer;
  char* FileNameMethod;
  int WriteAllTimeSteps;
  int NumberOfTimeSteps;
  int CurrentTimeIndex;

  // The name of the output file.
  char* FileName;
};

#endif

