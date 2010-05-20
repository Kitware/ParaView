/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawStridedReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRawStridedReader - read PNG files
// .SECTION Description
// vtkRawStridedReader is a source object that reads PNG files.
// It should be able to read most any PNG file
//
// .SECTION See Also
// vtkPNGWriter

#ifndef __vtkRawStridedReader_h
#define __vtkRawStridedReader_h

#include "vtkImageAlgorithm.h"

class vtkRawStridedReaderPiece;
class vtkRangeKeeper2;

class VTK_EXPORT vtkRawStridedReader : public vtkImageAlgorithm
{
public:
  static vtkRawStridedReader *New();
  vtkTypeMacro(vtkRawStridedReader,vtkImageAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  
  //By default the byte order is not swapped
  virtual void SwapDataByteOrder(int i);

  //virtual void SetFileName(const char *);
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

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkRawStridedReader();
  ~vtkRawStridedReader();

  virtual int RequestData(    
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  char *Filename;
  int WholeExtent[6];
  int Dimensions[3];
  double Origin[3];
  double Spacing[3];
  int Stride[3];

  int UpdateExtent[6];
  int BlockReadSize;
  vtkRawStridedReaderPiece *Internal;
  vtkRangeKeeper2 *RangeKeeper;

private:
  vtkRawStridedReader(const vtkRawStridedReader&);  // Not implemented.
  void operator=(const vtkRawStridedReader&);  // Not implemented.
};
#endif


