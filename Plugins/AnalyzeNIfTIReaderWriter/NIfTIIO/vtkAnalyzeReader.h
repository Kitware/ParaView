
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

#include "vtkAnalyzeNIfTIIOModule.h"
#include "vtkImageReader.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"
#define POINT_SPACE_ARRAY "vtkPointSpace"
#define VOLUME_ORIGIN_DOUBLE_ARRAY "vtkVolumeOrigin"
#define VOLUME_SPACING_DOUBLE_ARRAY "vtkVolumeSpacing"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkFieldData;

class VTKANALYZENIFTIIO_EXPORT vtkAnalyzeReader : public vtkImageReader
{
public:
  static vtkAnalyzeReader* New();
  vtkTypeMacro(vtkAnalyzeReader, vtkImageReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description: is the given file name a png file?
  int CanReadFile(const char* fname) override;

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  const char* GetFileExtensions() override { return ".img .hdr"; }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  const char* GetDescriptiveName() override { return "Analyze"; }

  char* GetFileName() override { return (FileName); };
  unsigned int getImageSizeInBytes() { return (imageSizeInBytes); };

protected:
  vtkAnalyzeReader();
  ~vtkAnalyzeReader() override;

  void ExecuteInformation() override;
  void ExecuteDataWithInformation(vtkDataObject* out, vtkInformation* outInfo) override;

private:
  vtkAnalyzeReader(const vtkAnalyzeReader&) = delete;
  void operator=(const vtkAnalyzeReader&) = delete;

  void vtkAnalyzeReaderUpdateVTKBit(vtkImageData* data, void* outPtr);

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
