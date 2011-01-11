/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMandelbrotSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamedMandelbrot - stream capable mandelbrot image generator
// .SECTION Description

#ifndef __vtkStreamedMandelbrot_h
#define __vtkStreamedMandelbrot_h

#include "vtkImageMandelbrotSource.h"

class vtkGridSampler1;
class vtkMetaInfoDatabase;

class VTK_EXPORT vtkStreamedMandelbrot : public vtkImageMandelbrotSource
{
public:
  static vtkStreamedMandelbrot *New();
  vtkTypeMacro(vtkStreamedMandelbrot,vtkImageMandelbrotSource);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkStreamedMandelbrot();
  ~vtkStreamedMandelbrot();

  virtual int ProcessRequest
    (vtkInformation *request,
     vtkInformationVector** inputVector,
     vtkInformationVector* outputVector);

  virtual int RequestData
    (vtkInformation *request,
     vtkInformationVector** inputVector,
     vtkInformationVector* outputVector);

  virtual int RequestInformation
    (vtkInformation *,
     vtkInformationVector**,
     vtkInformationVector *);

 private:
  vtkStreamedMandelbrot(const vtkStreamedMandelbrot&);  // Not implemented.
  void operator=(const vtkStreamedMandelbrot&);  // Not implemented.

  vtkGridSampler1 *GridSampler;
  vtkMetaInfoDatabase *RangeKeeper;
  double Resolution;
  int SI, SJ, SK;
};


#endif
