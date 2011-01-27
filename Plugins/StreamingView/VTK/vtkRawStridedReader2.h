/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawStridedReader2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRawStridedReader2 - reads raw binary files, will optionally subsample
// .SECTION Description
// vtkRawStridedReader2 is a source object that reads raw binary files. The files are
// assumed to contain nothing but floating point numbers. The caller must provide
// the spacing and extent in i, j, and k directions.
// This stride parameter, which tells the reader to subsample as it reads,
// reading every n'th value (in i, j, and/or k) to speed up file I/O and later
// processing in the pipeline.

#ifndef __vtkRawStridedReader2_h
#define __vtkRawStridedReader2_h

#include "vtkImageAlgorithm.h"

class vtkMetaInfoDatabase;
class vtkGridSampler2;

class VTK_EXPORT vtkRawStridedReader2 : public vtkImageAlgorithm
{
public:
  static vtkRawStridedReader2 *New();
  vtkTypeMacro(vtkRawStridedReader2,vtkImageAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  //By default the byte order is not swapped
  virtual void SwapDataByteOrder(int i);

  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);

  vtkSetVector3Macro(Spacing, double);
  vtkGetVector3Macro(Spacing, double);

  // Description:
  // Checks for presence of preprocessed files.
  int CanReadFile(const char *filename);

protected:
  vtkRawStridedReader2();
  ~vtkRawStridedReader2();

  int Read(float* data, int* uExtents);

  // Description:
  // Overridden to provide meta info when available and to catch whole extent requests
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

  // Description:
  // Reads file and produces requested data.
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // Overridden to produce meta information corresponding to requested resolution
  virtual int RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  char *Filename;
  int WholeExtent[6];
  double Origin[3];
  double Spacing[3];
  int SwapBytes;

  //actual produced resolution
  double Resolution;
  int sWholeExtent[6];
  double sSpacing[3];

  //Stores meta information as it is obtained.
  vtkMetaInfoDatabase *RangeKeeper;

  //Does resolution to stride mapping
  vtkGridSampler2 *GridSampler;

  // stuff to do memory maps
  FILE* fp;
  int fd;
  char* lastname;
  vtkIdType lastresolution;

#ifndef _WIN32
  int chunk;
  float* map;
  size_t mapsize;
  void TearDownMap();
  void SetupMap(int);
#endif

  void SetupFile();
  void TearDownFile();

private:
  vtkRawStridedReader2(const vtkRawStridedReader2&);  // Not implemented.
  void operator=(const vtkRawStridedReader2&);  // Not implemented.
};
#endif
