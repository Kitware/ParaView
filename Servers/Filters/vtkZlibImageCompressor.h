/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZlibImageCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkZlibImageCompressor - Image compressor/decompressor using Zlib.
// .SECTION Description
// This class compresses Image data using Zlib. The compression level
// varies between 1 and 9, 1 being the fastest at the cost of the
// compression ratio, 9 producing the highest compression ratio at the
// cost of speed. Optionally color depth may be reduced and alpha 
// stripped/restored.
// .SECTION Thanks
// SciberQuest Inc. contributed this class.

#ifndef __vtkZlibImageCompressor_h
#define __vtkZlibImageCompressor_h

#include "vtkImageCompressor.h"

class vtkZlibCompressorImageConditioner;
class vtkMultiProcessStream;

class VTK_EXPORT vtkZlibImageCompressor : public vtkImageCompressor
{
public:
  static vtkZlibImageCompressor* New();
  vtkTypeMacro(vtkZlibImageCompressor, vtkImageCompressor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Compress/Decompress data array on the objects input with results
  // in the objects output. See also Set/GetInput/Output.
  virtual int Compress();
  virtual int Decompress();

  //BTX
  // Description:
  // Serialize/Restore compressor configuration (but not the data) into the stream.
  virtual void SaveConfiguration(vtkMultiProcessStream *stream);
  virtual bool RestoreConfiguration(vtkMultiProcessStream* stream);
  //ETX
  virtual const char *SaveConfiguration();
  virtual const char *RestoreConfiguration(const char *stream);

  // Description:
  // Set compression level. A setting of 1 is the fastest producing the
  // smallest compression ratio while a setting of 9 is the slowest producing
  // the highest compression ratio. Zlib is loss-less regardless of level
  // however, setting SetColorSpaceReduction factor to a non zero value
  // will cause internal pre-processor to reduce the color space prior to
  // compression which can improve compression ratio realized.
  vtkSetClampMacro(CompressionLevel, int, 1, 9);
  vtkGetMacro(CompressionLevel, int);

  // Description:
  // Set to an integer between 0 and 5. This uses the same color space reduction
  // as the squirt compressor. If set to 0 no colorspace reduction is performed.
  void SetColorSpace(int csId);
  int GetColorSpace();

  // Description:
  // Set to boolean value indicating whether alpha values
  // should be stripped prior to compression. Stripping alpha values will reduce
  // input to compressor by 1/4 and results in speed up in compressor run time
  // and of course reduced image size. Stripped alpha value are reinstated to
  // 0xff during decompress.
  void SetStripAlpha(int status);
  int GetStripAlpha();

  // Description:
  // When set the implementation must use loss-less compression, otherwise
  // implemnetation should user provided settings.
  virtual void SetLossLessMode(int mode);

protected:
  vtkZlibImageCompressor();
  virtual ~vtkZlibImageCompressor();


private:
  vtkZlibCompressorImageConditioner *Conditioner; // manages color space reduction and strip alpha
  int CompressionLevel;                           // zlib compression level

private:
  vtkZlibImageCompressor(const vtkZlibImageCompressor&); // Not implemented.
  void operator=(const vtkZlibImageCompressor&); // Not implemented.
};

#endif
