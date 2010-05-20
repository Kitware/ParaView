/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunction
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
// .SECTION Description:
// vtk1DTransferFunction subclasses map an array to a 1D array.

#ifndef __vtk1DTransferFunction_h
#define __vtk1DTransferFunction_h

#include "vtkObject.h"
#include "vtkSetGet.h"

class vtkDataArray;

class VTK_EXPORT vtk1DTransferFunction: public vtkObject
{
public:
  vtkTypeMacro(vtk1DTransferFunction, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // map the input array to the output array using the Table.
  // the output will have 1 component and as many tuples as the input.
  virtual void  MapArray(vtkDataArray* input, vtkDataArray* output);

  // Set/Get the range of the input values
  vtkSetVector2Macro(InputRange, double);
  vtkGetVector2Macro(InputRange, double);

  vtkSetMacro(VectorComponent, int);
  vtkGetMacro(VectorComponent, int);

  vtkSetMacro(UseScalarRange, int);
  vtkGetMacro(UseScalarRange, int);
  vtkBooleanMacro(UseScalarRange, int);

  // map a value and store it in the output at the given index
  // using the Lookup Table
  virtual double  MapValue(double value, double* range) = 0;

protected:
  vtk1DTransferFunction();
  ~vtk1DTransferFunction();

  double InputRange[2];
  int VectorComponent;
  int UseScalarRange;

private:
  vtk1DTransferFunction(const vtk1DTransferFunction&); // Not implemented.
  void operator=(const vtk1DTransferFunction&); // Not implemented.
};

#endif

