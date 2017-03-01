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

#include "vtkImageWriter.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"

class vtkImageData;
class vtkUnsignedCharArray;

class vtkAnalyzeWriter : public vtkImageWriter
{
public:
  static vtkAnalyzeWriter* New();
  vtkTypeMacro(vtkAnalyzeWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void SetFileType(int inValue);
  int getFileType();

  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkAnalyzeWriter();
  ~vtkAnalyzeWriter();

  virtual void WriteFile(
    ofstream* file, vtkImageData* data, int ext[6], int wExtent[6]) VTK_OVERRIDE;
  virtual void WriteFileHeader(
    ofstream* file, vtkImageData* cache, int wholeExtent[6]) VTK_OVERRIDE;

private:
  vtkAnalyzeWriter(const vtkAnalyzeWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAnalyzeWriter&) VTK_DELETE_FUNCTION;

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
