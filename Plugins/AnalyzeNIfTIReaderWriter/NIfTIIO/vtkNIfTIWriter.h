// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Joseph Hennessey
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkNIfTIWriter - Writes NIfTI files.
// .SECTION Description
// vtkNIfTIWriter writes NIfTI files.
//
// .SECTION See Also
// vtkNIfTIReader vtkAnalayzeReader vtkAnalyzeWriter

#ifndef vtkNIfTIWriter_h
#define vtkNIfTIWriter_h

#include "vtkAnalyzeNIfTIIOModule.h" // for export macro
#include "vtkImageWriter.h"

#define NIFTI_HEADER_ARRAY "vtkNIfTIReaderHeaderArray"

class vtkDataArray;
class vtkFieldData;

class vtkImageData;
class vtkUnsignedCharArray;

class VTKANALYZENIFTIIO_EXPORT vtkNIfTIWriter : public vtkImageWriter
{
public:
  static vtkNIfTIWriter* New();
  vtkTypeMacro(vtkNIfTIWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetFileType(int inValue);
  int getFileType();

  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkNIfTIWriter();
  ~vtkNIfTIWriter() override;

  void WriteFile(ostream* file, vtkImageData* data, int ext[6], int wholeExtent[6]) override;
  void WriteFileHeader(ostream* file, vtkImageData* cache, int wholeExtent[6]) override;

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

  vtkNIfTIWriter(const vtkNIfTIWriter&) = delete;
  void operator=(const vtkNIfTIWriter&) = delete;
};

#endif
