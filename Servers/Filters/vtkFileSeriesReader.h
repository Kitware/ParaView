/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFileSeriesReader - meta-reader to read file series
// .SECTION Description:
// vtkFileSeriesReader is a meta-reader that can work with various
// readers to load file series. To the pipeline, it looks like a reader
// that supports time. It updates the file name to the internal reader
// whenever a different time step is requested.

#ifndef __vtkFileSeriesReader_h
#define __vtkFileSeriesReader_h

#include "vtkDataObjectAlgorithm.h"

//BTX
struct vtkFileSeriesReaderInternals;
//ETX

class VTK_EXPORT vtkFileSeriesReader : public vtkDataObjectAlgorithm
{
public:
  static vtkFileSeriesReader* New();
  vtkTypeRevisionMacro(vtkFileSeriesReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the internal reader.
  void SetReader(vtkAlgorithm*);
  vtkGetObjectMacro(Reader, vtkAlgorithm);

  // Description:
  // All pipeline passes are forwarded to the internal reader. The
  // vtkFileSeriesReader reports time steps in RequestInformation. It
  // updated the file name of the internal in RequestUpdateExtent based
  // on the time step request.
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // CanReadFile is forwarded to the internal reader if it supports it.
  int CanReadFile(const char* name);

  // Description:
  // Adds names of files to be read. The files are read in the order
  // they are added.
  void AddFileName(const char* fname);

  // Description:
  // Remove all file names.
  void RemoveAllFileNames();

  // Description:
  // Returns the number of file names added by AddFileName.
  unsigned int GetNumberOfFileNames();

  // Description:
  // Returns the name of a file with index idx.
  const char* GetFileName(unsigned int idx);

  // Description:
  // Return the MTime also considering the internal reader.
  unsigned long GetMTime();

  // Description:
  // Name of the method used to set the file name of the internal
  // reader. By default, this is SetFileName.
  vtkSetStringMacro(FileNameMethod);
  vtkGetStringMacro(FileNameMethod);

protected:
  vtkFileSeriesReader();
  ~vtkFileSeriesReader();

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  void SetReaderFileName(const char* fname);
  vtkAlgorithm* Reader;

  char* FileNameMethod;

private:
  vtkFileSeriesReader(const vtkFileSeriesReader&); // Not implemented.
  void operator=(const vtkFileSeriesReader&); // Not implemented.

  vtkFileSeriesReaderInternals* Internal;
};

#endif

