
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnalyzeReader.h

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAnalyzeReader - read Analyze files
// .SECTION Description
// vtkAnalyzeReader is a source object that reads Analyze files.
// It should be able to read most any Analyze file
//
// .SECTION See Also
// vtkAnalyzeWriter vtkNIfTIReader vtkNIfTIWriter

#ifndef vtkAnalyzeReader_h
#define vtkAnalyzeReader_h

#include "vtkImageReader.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"
#define POINT_SPACE_ARRAY "vtkPointSpace"
#define VOLUME_ORIGIN_DOUBLE_ARRAY "vtkVolumeOrigin"
#define VOLUME_SPACING_DOUBLE_ARRAY "vtkVolumeSpacing"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkFieldData;

class vtkAnalyzeReader : public vtkImageReader
{
public:
  static vtkAnalyzeReader* New();
  vtkTypeMacro(vtkAnalyzeReader, vtkImageReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description: is the given file name a png file?
  virtual int CanReadFile(const char* fname) VTK_OVERRIDE;

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions() VTK_OVERRIDE { return ".img .hdr"; }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName() VTK_OVERRIDE { return "Analyze"; }

  char* GetFileName() VTK_OVERRIDE { return (FileName); };
  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkAnalyzeReader();
  ~vtkAnalyzeReader();

  virtual void ExecuteInformation() VTK_OVERRIDE;
  virtual void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) VTK_OVERRIDE;

private:
  vtkAnalyzeReader(const vtkAnalyzeReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAnalyzeReader&) VTK_DELETE_FUNCTION;

  void vtkAnalyzeReaderUpdateVTKBit(vtkImageData* data, void* outPtr);

  unsigned int numberOfDimensions;
  unsigned int imageSizeInBytes;
  unsigned int orientation;
  double dataTypeSize;
  unsigned int Type;
  int voxelDimensions[3];
  int diskDimensions[3];
  int diskExtent[6];
  double diskSpacing[3];
  // int width;
  // int height;
  // int depth;
  int binaryOnDiskWidth;
  int binaryOnDiskHeight;
  int binaryOnDiskDepth;

  vtkUnsignedCharArray* analyzeHeader;
  unsigned char* analyzeHeaderUnsignedCharArray;
  int analyzeHeaderSize;

  bool fixFlipError;
};
#endif
