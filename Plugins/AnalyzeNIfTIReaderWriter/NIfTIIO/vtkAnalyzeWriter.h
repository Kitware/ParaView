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

#ifndef vtkAnalyzeWriter_h
#define vtkAnalyzeWriter_h

#include "vtkAnalyzeNIfTIIOModule.h"
#include "vtkImageWriter.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"

class vtkImageData;
class vtkUnsignedCharArray;

class VTKANALYZENIFTIIO_EXPORT vtkAnalyzeWriter : public vtkImageWriter
{
public:
  static vtkAnalyzeWriter* New();
  vtkTypeMacro(vtkAnalyzeWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetFileType(int inValue);
  int getFileType();

  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkAnalyzeWriter();
  ~vtkAnalyzeWriter() override;

  void WriteFile(ostream* file, vtkImageData* data, int ext[6], int wExtent[6]) override;
  void WriteFileHeader(ostream* file, vtkImageData* cache, int wholeExtent[6]) override;

private:
  vtkAnalyzeWriter(const vtkAnalyzeWriter&) = delete;
  void operator=(const vtkAnalyzeWriter&) = delete;

  int FileType;
  unsigned int imageSizeInBytes;
  unsigned int orientation;
  double dataTypeSize;
  int imageDataType;
  bool foundAnalayzeHeader;
  bool foundNiftiHeader;
  int* savedFlipAxis;
  int* savedInPlaceFilteredAxes;
  bool fixFlipError;
};

#endif
