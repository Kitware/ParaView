// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkSystemIncludes.h"               // for istream
#include "vtkType.h"                         // for vtkTypeInt64

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
