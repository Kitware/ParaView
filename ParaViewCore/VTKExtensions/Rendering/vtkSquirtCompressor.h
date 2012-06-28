/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquirtCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkSquirtCompressor - Image compressor/decompressor using SQUIRT.
// .SECTION Description
// This class compresses Image data using SQUIRT a Run-Length-Encoded
// compression scheme. The Squirt Level controls the compression. 0 is
// lossless compression, 1 through 5 are lossy compression levels with
// 5 being maximum compression.
//
// Squirt produces smaller compression ratio than some other popular
// compression algorithm. However, Squirt has a relatively high
// throughput compared to some other compression algorithm. Squirt's
// performance is optimized for RGBa images, however the class can
// also work with RGB images. There is no performance hit when applying
// the lossy comrpession levels.
//
// Levels 1 through 5 apply a color reducing mask to the run computation,
// not to the pixel directly. This is clever in that no new colors are
// introduced to the image, and as a result one doesn't see drastic changes
// between the reduced color image and the original. However, when using
// the higher levels one may get runs that produce visual artifiacts. For
// example when a run starts in one actor whose reduced color matches the
// background the background is colored with the actor color.
//
// .SECTION Thanks
// Thanks to Sandia National Laboratories for this compression technique

#ifndef __vtkSquirtCompressor_h
#define __vtkSquirtCompressor_h

#include "vtkImageCompressor.h"

class vtkMultiProcessStream;

class VTK_EXPORT vtkSquirtCompressor : public vtkImageCompressor
{
public:
  static vtkSquirtCompressor* New();
  vtkTypeMacro(vtkSquirtCompressor, vtkImageCompressor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set Squirt compression level.
  // Level 0 is lossless compression, 1 through 5 are lossy compression
  // levels with 5 being maximum compression.
  vtkSetClampMacro(SquirtLevel, int, 0, 5);
  vtkGetMacro(SquirtLevel, int);

  // Description:
  // Compress/Decompress data array on the objects input with results
  // in the objects output. See also Set/GetInput/Output.
  virtual int Compress();
  virtual int Decompress();

  //BTX
  // Description:
  // Serialize/Restore compressor configuration (but not the data) into the stream.
  virtual void SaveConfiguration(vtkMultiProcessStream *stream);
  virtual bool RestoreConfiguration(vtkMultiProcessStream *stream);
  //ETX
  virtual const char *SaveConfiguration();
  virtual const char *RestoreConfiguration(const char *stream);

protected:
  vtkSquirtCompressor();
  virtual ~vtkSquirtCompressor();

  int SquirtLevel;

private:
  vtkSquirtCompressor(const vtkSquirtCompressor&); // Not implemented.
  void operator=(const vtkSquirtCompressor&); // Not implemented.
};


#endif
