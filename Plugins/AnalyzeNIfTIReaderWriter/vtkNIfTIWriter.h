/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIfTIWriter.h

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNIfTIWriter - Writes NIfTI files.
// .SECTION Description
// vtkNIfTIWriter writes NIfTI files.
//
// .SECTION See Also
// vtkNIfTIReader vtkAnalayzeReader vtkAnalyzeWriter

#ifndef vtkNIfTIWriter_h
#define vtkNIfTIWriter_h

#include "vtkImageWriter.h"

#define NIFTI_HEADER_ARRAY "vtkNIfTIReaderHeaderArray"

class vtkDataArray;
class vtkFieldData;

class vtkImageData;
class vtkUnsignedCharArray;

class vtkNIfTIWriter : public vtkImageWriter
{
public:
  static vtkNIfTIWriter* New();
  vtkTypeMacro(vtkNIfTIWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void SetFileType(int inValue);
  int getFileType();

  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkNIfTIWriter();
  ~vtkNIfTIWriter();

  virtual void WriteFile(
    ofstream* file, vtkImageData* data, int ext[6], int wholeExtent[6]) VTK_OVERRIDE;
  virtual void WriteFileHeader(
    ofstream* file, vtkImageData* cache, int wholeExtent[6]) VTK_OVERRIDE;

private:
  int FileType;
  unsigned int imageSizeInBytes;
  double dataTypeSize;
  int iname_offset;
  bool foundNiftiHeader;
  bool foundAnalayzeHeader;
  double** q;
  double** s;
  int sform_code;
  int qform_code;

  vtkNIfTIWriter(const vtkNIfTIWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNIfTIWriter&) VTK_DELETE_FUNCTION;
};

#endif
