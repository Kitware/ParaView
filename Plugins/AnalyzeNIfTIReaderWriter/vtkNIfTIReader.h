/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIfTIReader.h

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkNIfTIReader - read NIfTI files
// .SECTION Description
// vtkNIfTIReader is a source object that reads NIfTI files.
// It should be able to read most any NIfTI file
//
// .SECTION See Also
// vtkNIfTIWriter vtkAnalyzeReader vtkAnalyzeWriter

#ifndef vtkNIfTIReader_h
#define vtkNIfTIReader_h

#include "vtkImageReader.h"

#define NIFTI_HEADER_ARRAY "vtkNIfTIReaderHeaderArray"
#define POINT_SPACE_ARRAY "vtkPointSpace"
#define VOLUME_ORIGIN_DOUBLE_ARRAY "vtkVolumeOrigin"
#define VOLUME_SPACING_DOUBLE_ARRAY "vtkVolumeSpacing"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkFieldData;

class vtkNIfTIReader : public vtkImageReader
{
public:
  static vtkNIfTIReader* New();
  vtkTypeMacro(vtkNIfTIReader, vtkImageReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description: is the given file name a png file?
  virtual int CanReadFile(const char* fname) VTK_OVERRIDE;

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions() VTK_OVERRIDE { return ".nii .img .hdr"; }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName() VTK_OVERRIDE { return "NIfTI"; }

  char* GetFileName() VTK_OVERRIDE { return (FileName); };
  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkNIfTIReader();
  ~vtkNIfTIReader();

  virtual void ExecuteInformation() VTK_OVERRIDE;
  virtual void ExecuteDataWithInformation(
    vtkDataObject* output, vtkInformation* outInfo) VTK_OVERRIDE;

private:
  vtkNIfTIReader(const vtkNIfTIReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNIfTIReader&) VTK_DELETE_FUNCTION;

  unsigned int numberOfDimensions;
  unsigned int imageSizeInBytes;
  unsigned int Type;
  int width;
  int height;
  int depth;

  double dataTypeSize;
  double** q;
  double** s;
  int sform_code;
  int qform_code;
  int niftiType;

  vtkUnsignedCharArray* niftiHeader;
  unsigned char* niftiHeaderUnsignedCharArray;
  int niftiHeaderSize;
};
#endif
