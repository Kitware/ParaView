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
// .NAME vtkSquirtCompressor - Image compressor/decompressor using SQUIRT.
// .SECTION Description
// This class compresses Image data using SQUIRT. The Squirt
// Level controls the compression. 0 is no compression and 5 is
// maximum compression.

#ifndef __vtkSquirtCompressor_h
#define __vtkSquirtCompressor_h

#include "vtkImageCompressor.h"

class VTK_EXPORT vtkSquirtCompressor : public vtkImageCompressor
{
public:
  static vtkSquirtCompressor* New();
  vtkTypeRevisionMacro(vtkSquirtCompressor, vtkImageCompressor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set Squirt compression level.
  // Level 0 means no compression(only RLE encoding) and 5 is maximum.
  // Default value is 0.
  vtkSetClampMacro(SquirtLevel, int, 0, 5);
  vtkGetMacro(SquirtLevel, int);
protected:
  vtkSquirtCompressor();
  ~vtkSquirtCompressor();

  virtual int CompressData();
  virtual int DecompressData();

  int SquirtLevel;

private:
  vtkSquirtCompressor(const vtkSquirtCompressor&); // Not implemented.
  void operator=(const vtkSquirtCompressor&); // Not implemented.
    
};


#endif
