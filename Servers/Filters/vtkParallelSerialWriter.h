/*=========================================================================

  Program:   ParaView
  Module:    vtkParallelSerialWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkParallelSerialWriter - parallel meta-writer for serial formats
// .SECTION Description:
// vtkParallelSerialWriter is a meta-writer that enables serial writers
// to work in parallel. It gathers data to the 1st node and invokes the
// internal writer. Currently, only polydata is supported.

#ifndef __vtkParallelSerialWriter_h
#define __vtkParallelSerialWriter_h

#include "vtkDataObjectAlgorithm.h"

class vtkPolyData;

class VTK_EXPORT vtkParallelSerialWriter : public vtkDataObjectAlgorithm
{
public:
  static vtkParallelSerialWriter* New();
  vtkTypeRevisionMacro(vtkParallelSerialWriter, vtkDataObjectAlgorithm);
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
  // Get/Set the piece number to write.  The same piece number is used
  // for all inputs.
  vtkGetMacro(Piece, int);
  vtkSetMacro(Piece, int);
  
  // Description:
  // Get/Set the number of pieces into which the inputs are split.
  vtkGetMacro(NumberOfPieces, int);
  vtkSetMacro(NumberOfPieces, int);
  
  // Description:
  // Get/Set the number of ghost levels to be written.
  vtkGetMacro(GhostLevel, int);
  vtkSetMacro(GhostLevel, int);

protected:
  vtkParallelSerialWriter();
  ~vtkParallelSerialWriter();

  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);

private:
  vtkParallelSerialWriter(const vtkParallelSerialWriter&); // Not implemented.
  void operator=(const vtkParallelSerialWriter&); // Not implemented.
  
  void WriteAFile(const char* fname, vtkPolyData* input);

  void SetWriterFileName(const char* fname);
  void WriteInternal();

  vtkAlgorithm* Writer;
  char* FileNameMethod;
  int Piece;
  int NumberOfPieces;
  int GhostLevel;

  // The name of the output file.
  char* FileName;
};

#endif

