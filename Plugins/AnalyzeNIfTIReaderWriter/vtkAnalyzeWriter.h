/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnalyzeWriter.h

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnalyzeWriter - Writes Analyze files.
// .SECTION Description
// vtkAnalyzeWriter writes Analyze files. 
//
// .SECTION See Also
// vtkAnalyzeReader vtkNIfTIReader vtkNIfTIWriter

#ifndef __vtkAnalyzeWriter_h
#define __vtkAnalyzeWriter_h

#include "vtkImageWriter.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"

class vtkImageData;
class vtkUnsignedCharArray;

class vtkAnalyzeWriter : public vtkImageWriter
{
public:
  static vtkAnalyzeWriter *New();
  vtkTypeMacro(vtkAnalyzeWriter,vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  void SetFileType(int inValue);
  int getFileType();

  unsigned int getImageSizeInBytes(){return(imageSizeInBytes);};

protected:
  vtkAnalyzeWriter();
  ~vtkAnalyzeWriter();
  
  virtual void WriteFile(ofstream *file, vtkImageData *data, int ext[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *);
private:

  int FileType;
  unsigned int imageSizeInBytes;
  unsigned int orientation;
  double dataTypeSize;
  int imageDataType;
  bool foundAnalayzeHeader;
  bool foundNiftiHeader;
  int * savedFlipAxis;
  int * savedInPlaceFilteredAxes;

  vtkAnalyzeWriter(const vtkAnalyzeWriter&);  // Not implemented.
  void operator=(const vtkAnalyzeWriter&);  // Not implemented.
};

#endif


