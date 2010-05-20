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

#ifndef __vtkAnalyzeReader_h
#define __vtkAnalyzeReader_h

#include "vtkImageReader.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"

class vtkDataArray;
class vtkUnsignedCharArray;
class vtkFieldData;

class vtkAnalyzeReader : public vtkImageReader
{
public:
  static vtkAnalyzeReader *New();
  vtkTypeMacro(vtkAnalyzeReader,vtkImageReader);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description: is the given file name a png file?
  virtual int CanReadFile(const char* fname);

  // Description:
  // Get the file extensions for this format.
  // Returns a string with a space separated list of extensions in
  // the format .extension
  virtual const char* GetFileExtensions()
    {
      return ".img .hdr";
    }

  // Description:
  // Return a descriptive name for the file format that might be useful in a GUI.
  virtual const char* GetDescriptiveName()
    {
      return "Analyze";
    }

   char * GetFileName(){return(FileName);};
   unsigned int getImageSizeInBytes(){return(imageSizeInBytes);};
  
protected:
  vtkAnalyzeReader();
  ~vtkAnalyzeReader();

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *out);
private:
  vtkAnalyzeReader(const vtkAnalyzeReader&);  // Not implemented.
  void operator=(const vtkAnalyzeReader&);  // Not implemented.

  void vtkAnalyzeReaderUpdateVTKBit(vtkImageData *data, void *outPtr);

  unsigned int numberOfDimensions;
  unsigned int imageSizeInBytes;
  unsigned int orientation;
  double dataTypeSize;
  unsigned int Type;
  int width;
  int height;
  int depth;
  int binaryOnDiskWidth;
  int binaryOnDiskHeight;
  int binaryOnDiskDepth;

  vtkUnsignedCharArray *analyzeHeader;
  unsigned char * analyzeHeaderUnsignedCharArray;
  int analyzeHeaderSize;

};
#endif


