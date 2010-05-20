/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DLookupTableTransferFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DLookupTableTransferFunction
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
// .SECTION Description - 1D transfer function that uses a lookup table
// to map the values.

#ifndef __vtk1DLookupTableTransferFunction_h
#define __vtk1DLookupTableTransferFunction_h

#include "vtk1DTransferFunction.h"

class vtkDoubleArray;

class VTK_EXPORT vtk1DLookupTableTransferFunction: public vtk1DTransferFunction
{
public:
  static vtk1DLookupTableTransferFunction* New();
  vtkTypeMacro(vtk1DLookupTableTransferFunction, vtk1DTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If this flag is on, values will be linearly interpolated between the
  // two nearest table values.
  // Otherwise, the returned value will be the one below.
  vtkSetMacro(Interpolation, int);
  vtkGetMacro(Interpolation, int);

  // Description:
  // Set/Get the size of the lookup table.
  virtual void  SetNumberOfTableValues(vtkIdType);
  virtual vtkIdType  GetNumberOfTableValues();

  // Description:
  // Set/Get a single table value.
  virtual void  SetTableValue(vtkIdType, double);
  virtual double  GetTableValue(vtkIdType);

  // Description:
  // clean the lookup table.
  virtual void  RemoveAllTableValues();

  // Description:
  // This will build a lookup table
  virtual void  BuildDefaultTable();

  // map a value and store it in the output at the given index
  // using the Lookup Table
  virtual double  MapValue(double value, double* range);

protected:
  vtk1DLookupTableTransferFunction();
  ~vtk1DLookupTableTransferFunction();

  vtkDoubleArray* Table;
  int Interpolation;

private:
  vtk1DLookupTableTransferFunction(const vtk1DLookupTableTransferFunction&); // Not implemented.
  void operator=(const vtk1DLookupTableTransferFunction&); // Not implemented.
};

#endif

