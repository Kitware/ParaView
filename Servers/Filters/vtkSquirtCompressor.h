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
// This class compresses Image data using SQUIRT. The Squirt
// Level controls the compression. 0 is lossless compression, 1 through
// 5 are lossy compression levels with 5 being maximum compression.
// .SECTION Thanks
// Thanks to Sandia National Laboratories for this compression technique

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
  // Level 0 is lossless compression, 1 through
  // 5 are lossy compression levels with 5 being maximum compression.
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
