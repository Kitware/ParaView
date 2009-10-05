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

class vtkRSRFileSkimmer2;
class vtkMetaInfoDatabase;
class vtkGridSampler2;

class VTK_EXPORT vtkRawStridedReader2 : public vtkImageAlgorithm
{
public:
  static vtkRawStridedReader2 *New();
  vtkTypeRevisionMacro(vtkRawStridedReader2,vtkImageAlgorithm);
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

  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);

  vtkSetMacro(BlockReadSize, int);
  vtkGetMacro(BlockReadSize, int);

  float* SetupMap(size_t);

protected:
  vtkRawStridedReader2();
  ~vtkRawStridedReader2();

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

  // Description:
  // Overridden for debugging.
  virtual int RequestUpdateExtent(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  char *Filename;
  int WholeExtent[6];
  int sWholeExtent[6];
  int Dimensions[3];
  double Origin[3];
  double Spacing[3];

  //user requested stride
  int Stride[3];
  int UpdateExtent[6];

  //actual produced resolution
  double Resolution;

  //computed stride
  int SI;
  int SJ;
  int SK;

  //buffer size to read off of disk into
  int BlockReadSize;

  //Does file I/O
  vtkRSRFileSkimmer2 *Skimmer;

  //Stores meta information as it is obtained.
  vtkMetaInfoDatabase *RangeKeeper;

  //Does resolution to stride mapping
  vtkGridSampler2 *GridSampler;
  
  // stuff to do memory maps
  FILE* fp;
  int fd;
  char* lastname;
  vtkIdType lastresolution;

  size_t chunk;
  float* map;
  size_t mapsize;

  void SetupFile();
  void TearDownFile();
  void TearDownMap();

private:
  vtkRawStridedReader2(const vtkRawStridedReader2&);  // Not implemented.
  void operator=(const vtkRawStridedReader2&);  // Not implemented.
};
#endif


