/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridSampler2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridSampler2 - A function that maps data extent and requested
// resolution and data extent to i,j,k stride choice.
// .SECTION Description
// Supply this with a whole extent, then give it a resolution request. It
// computes stride choices to match that resolution. Resolution of 0.0 is
// defined as requesting minimal resolution, resolution of 1.0 is defined
// as a request for full resolution (stride = 1,1,1).
//
// TODO: cache results!
//  separate out split path and stride determination
//  allow user specified stride and split determination
//  publish max splits via a Get method

#ifndef __vtkGridSampler2_h
#define __vtkGridSampler2_h

#include "vtkObject.h"

class vtkIntArray;

class VTK_EXPORT vtkGridSampler2 : public vtkObject
{
public:
  static vtkGridSampler2 *New();
  vtkTypeMacro(vtkGridSampler2,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetWholeExtent(int *);
  vtkIntArray *GetSplitPath();

  void SetSpacing(double *);
  void ComputeAtResolution(double r);

  void GetStrides(int *);
  void GetStridedExtent(int *);
  void GetStridedSpacing(double *);
  double GetStridedResolution();

protected:
  vtkGridSampler2();
  ~vtkGridSampler2();

  double SuggestSampling(int axis);
  void ComputeSplits(int *spLen, int **sp);


  int WholeExtent[6];
  double Spacing[3];
  double RequestedResolution;

  bool PathValid;
  bool SamplingValid;

  vtkIntArray *SplitPath;
  int Strides[3];
  int StridedExtent[6];
  double StridedResolution;
  double StridedSpacing[3];

private:
  vtkGridSampler2(const vtkGridSampler2&);  // Not implemented.
  void operator=(const vtkGridSampler2&);
};
#endif
