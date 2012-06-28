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
//
// .SECTION Description
// vtkImageCompressor is an abstract superclass for the helper object
// used to compress images by the vtkParallelManager subclasses.
// Compressors must implement Compress,Decomperss methods, which respect
// the LossLessMode ivar, which is used by the composite manager to force
// loss less compression during a still render. Additionally compressors
// must be able to seriealize and restore their setting from a stream.

#ifndef __vtkImageCompressor_h
#define __vtkImageCompressor_h

#include "vtkObject.h"

class vtkUnsignedCharArray;
class vtkMultiProcessStream;

class VTK_EXPORT vtkImageCompressor : public vtkObject
{
public:
  vtkTypeMacro(vtkImageCompressor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the input to this compressor. 
  void SetInput(vtkUnsignedCharArray* input);
  vtkGetObjectMacro(Input, vtkUnsignedCharArray);

  // Description:
  // Get/Set the output of the compressor.
  vtkGetObjectMacro(Output, vtkUnsignedCharArray);
  void SetOutput(vtkUnsignedCharArray*);

  // Description:
  // When set the implementation must use loss-less compression, otherwise
  // implemnetation should user provided settings.
  vtkSetMacro(LossLessMode,int);
  vtkGetMacro(LossLessMode,int);

  // Description:
  // Call this method to compress the input and generate the compressed
  // data.
  virtual int Compress()=0;

  // Description:
  // Decompresses and geenartes the decompressed data as output.
  // Input must be compressed data.
  virtual int Decompress()=0;

  //BTX
  // Description:
  // Serialize compressor configuration (but not the data) into the stream.
  virtual void SaveConfiguration(vtkMultiProcessStream *stream);

  // Description:
  // Restore state from the stream. The stream format for all image compressor
  // is: [ClassName, LossLessMode, [Derived Class Stream]].
  virtual bool RestoreConfiguration(vtkMultiProcessStream* stream);
  //ETX

  // Description:
  // Serialize compressor configuration (but not the data) into the stream.
  // A pointer to the internally managed stream is returned (ie do not free it!).
  virtual const char *SaveConfiguration();

  // Description:
  // Restore state from the stream, The stream format for all image compressor
  // is: [ClassName, LossLessMode, [Derived Class Stream]].
  // Upon success the stream is returned otherwise 0 is returned indicating
  // an error.
  virtual const char *RestoreConfiguration(const char *stream);

protected:
  // Description:
  // Construct with NULL input array and empty but allocated output array.
  vtkImageCompressor();
  virtual ~vtkImageCompressor();

  // This is the array which contains the compressed data.
  vtkUnsignedCharArray* Output;
  vtkUnsignedCharArray* Input;

  int LossLessMode;

  vtkSetStringMacro(Configuration);
  char *Configuration;

private:
  vtkImageCompressor(const vtkImageCompressor&); // Not implemented.
  void operator=(const vtkImageCompressor&); // Not implemented.
};

#endif
