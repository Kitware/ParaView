/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotIStream.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpyPlotIStream
 *
 * vtkSpyPlotIStream represents input functionality required by
 * the vtkSpyPlotReader and vtkSpyPlotUniReader classes.  The class
 * was factored out of vtkSpyPlotReader.cxx.  The class wraps an already
 * opened istream
 *
*/

#ifndef vtkSpyPlotIStream_h
#define vtkSpyPlotIStream_h
#include "vtkPVVTKExtensionsIOSPCTHModule.h" //needed for exports
#include "vtkSystemIncludes.h"
#include "vtkType.h"

class VTKPVVTKEXTENSIONSIOSPCTH_EXPORT vtkSpyPlotIStream
{
public:
  vtkSpyPlotIStream();
  virtual ~vtkSpyPlotIStream();
  void SetStream(istream*);
  istream* GetStream();
  int ReadString(char* str, size_t len);
  int ReadString(unsigned char* str, size_t len);
  int ReadInt32s(int* val, int num);
  int ReadInt32sNoSwap(int* val, int num);
  int ReadInt64s(vtkTypeInt64* val, int num);
  int ReadDoubles(double* val, int num);
  void Seek(vtkTypeInt64 offset, bool rel = false);
  vtkTypeInt64 Tell();

protected:
  const int FileBufferSize;
  char* Buffer;
  istream* IStream;

private:
  vtkSpyPlotIStream(const vtkSpyPlotIStream&) = delete;
  void operator=(const vtkSpyPlotIStream&) = delete;
};

inline istream* vtkSpyPlotIStream::GetStream()
{
  return this->IStream;
}

#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotIStream.h
