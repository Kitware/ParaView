/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DGaussianTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DGaussianTransferFunction
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>
// .SECTION Description - 1D transfer function that maps values
// to the max of a set af gaussians

#ifndef __vtk1DGaussianTransferFunction_h
#define __vtk1DGaussianTransferFunction_h

#include "vtk1DTransferFunction.h"

class vtkDoubleArray;

class VTK_EXPORT vtk1DGaussianTransferFunction: public vtk1DTransferFunction
{
public:
  static vtk1DGaussianTransferFunction* New();
  vtkTypeMacro(vtk1DGaussianTransferFunction, vtk1DTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the number of Gaussian control points.
  virtual void  SetNumberOfGaussianControlPoints(vtkIdType);
  virtual vtkIdType  GetNumberOfGaussianControlPoints();

  // Description:
  // Set/Get a single Gaussian control point.
  virtual void  SetGaussianControlPoint(vtkIdType, double, double, double, double, double);
  virtual void  SetGaussianControlPoint(vtkIdType, double*);
  virtual void  GetGaussianControlPoint(vtkIdType, double*);

  // Description:
  // add/remove a Gaussian control point.
  virtual void  AddGaussianControlPoint(double, double, double, double, double);
  virtual void  AddGaussianControlPoint(double*);
  virtual void  RemoveGaussianControlPoint(vtkIdType);

  // Description:
  // clean all control points.
  virtual void  RemoveAllGaussianControlPoints();

  // map a value and store it in the output at the given index
  // using the Lookup Table
  virtual double  MapValue(double value, double* range);

protected:
  vtk1DGaussianTransferFunction();
  ~vtk1DGaussianTransferFunction();

  vtkDoubleArray* GaussianControlPoints;

private:
  vtk1DGaussianTransferFunction(const vtk1DGaussianTransferFunction&); // Not implemented.
  void operator=(const vtk1DGaussianTransferFunction&); // Not implemented.
};

#endif

