/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageCompressor - Superclass for image compressor/decompressor
// used by Composite Managers.
// .SECTION Description
// vtkImageCompressor is an abstract superclass for the helper object
// used to compress images by the vtkParallelManager subclasses.

#ifndef __vtkImageCompressor_h
#define __vtkImageCompressor_h

#include "vtkObject.h"

class vtkUnsignedCharArray;

class VTK_EXPORT vtkImageCompressor : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkImageCompressor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the input to this compressor. 
  void SetInput(vtkUnsignedCharArray* input);
  vtkGetObjectMacro(Input, vtkUnsignedCharArray);

  // Description:
  // Call this method to compress the input and generate the compressed 
  // data.
  int Compress();
 
  // Description:
  // Decompresses and geenartes the decompressed data as output.
  // Input must be compressed data.
  int Decompress();

  // Description:
  // Get/Set the output of the compressor.
  vtkGetObjectMacro(Output, vtkUnsignedCharArray);
  void SetOutput(vtkUnsignedCharArray*);

protected:
  vtkImageCompressor();
  ~vtkImageCompressor();

  // Description:
  // Subclass override this method to perform actual compression.
  virtual int CompressData() = 0;

  // Description:
  // Subclass must override to perform actual decompression.
  virtual int DecompressData() = 0;

  // This is the array which contains the compressed data.
  vtkUnsignedCharArray* Output;
  vtkUnsignedCharArray* Input;

private:
  vtkImageCompressor(const vtkImageCompressor&); // Not implemented.
  void operator=(const vtkImageCompressor&); // Not implemented.
};

#endif
