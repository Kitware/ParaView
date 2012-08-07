/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrthogonalSliceFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOrthogonalSliceFilter - Cut vtkDataSet with 3 orthogonal planes
// .SECTION Description
// vtkOrthogonalSliceFilter is a filter to cut through data using 3 orthogonal
// plane that could be duplicated along each axis.

#ifndef __vtkOrthogonalSliceFilter_h
#define __vtkOrthogonalSliceFilter_h

#include "vtkPolyDataAlgorithm.h"

class vtkCutter;
class vtkAppendPolyData;

class vtkOrthogonalSliceFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkOrthogonalSliceFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0; and
  // generating cut scalars turned off.
  static vtkOrthogonalSliceFilter *New();

  // Description:
  // Override GetMTime because we rely on internal filters that have their own MTime
  unsigned long GetMTime();

  // Description:
  // Manage slices normal to X
  void SetSliceX(int index, double sliceValue);
  void SetNumberOfSliceX(int size);

  // Description:
  // Manage slices normal to Y
  void SetSliceY(int index, double sliceValue);
  void SetNumberOfSliceY(int size);

  // Description:
  // Manage slices normal to Z
  void SetSliceZ(int index, double sliceValue);
  void SetNumberOfSliceZ(int size);

protected:
  vtkOrthogonalSliceFilter();
  ~vtkOrthogonalSliceFilter();

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkCutter* SliceAlongX;
  vtkCutter* SliceAlongY;
  vtkCutter* SliceAlongZ;
  vtkAppendPolyData* CombinedFilteredInput;

  void SetSlice(vtkCutter* slice, int index, double value);
  void SetNumberOfSlice(vtkCutter* slice, int size);

private:
  vtkOrthogonalSliceFilter(const vtkOrthogonalSliceFilter&);  // Not implemented.
  void operator=(const vtkOrthogonalSliceFilter&);  // Not implemented.
};

#endif
