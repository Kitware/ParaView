/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk1DTransferFunctionChooser.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtk1DTransferFunctionChooser
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
// .SECTION Description
// Chooses between vtk1DLookupTableTransferFunction and vtk1DGaussianTransferFunction

#ifndef __vtk1DTransferFunctionChooser_h
#define __vtk1DTransferFunctionChooser_h

#include "vtk1DTransferFunction.h"


class vtk1DLookupTableTransferFunction;
class vtk1DGaussianTransferFunction;
class vtkProportionalTransferFunction;

class VTK_EXPORT vtk1DTransferFunctionChooser: public vtk1DTransferFunction
{
public:
  static vtk1DTransferFunctionChooser* New();
  vtkTypeMacro(vtk1DTransferFunctionChooser, vtk1DTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum
  {
  LookupTable = 0, Gaussian = 1
  };
  //ETX
  // 0 : LookupTable
  // 1 : Gaussian
  vtkSetMacro(TransferFunctionMode, int);
  vtkGetMacro(TransferFunctionMode, int);
  void  SetTransferFunctionModeToLookupTable()
  {
  this->SetTransferFunctionMode(LookupTable);
  }
  void  SetTransferFunctionModeToGaussian()
  {
  this->SetTransferFunctionMode(Gaussian);
  }

  void  SetLookupTableTransferFunction(vtk1DLookupTableTransferFunction*);
  vtkGetObjectMacro(LookupTableTransferFunction, vtk1DLookupTableTransferFunction);

  void  SetGaussianTransferFunction(vtk1DGaussianTransferFunction*);
  vtkGetObjectMacro(GaussianTransferFunction, vtk1DGaussianTransferFunction);

  virtual void  BuildDefaultTransferFunctions();

  // Description:
  // map the input array to the output array using the Table.
  // the output will have 1 component and as many tuples as the input.
  virtual void  MapArray(vtkDataArray* input, vtkDataArray* output);

  // map a value and store it in the output at the given index
  // using the Lookup Table
  virtual double  MapValue(double value, double* range);

  // overloaded to take into account the sub Transfet functions
  virtual unsigned long  GetMTime();

  virtual void SetInputRange(double*);
  virtual void SetInputRange(double, double);
  virtual void SetVectorComponent(int);
  virtual void SetUseScalarRange(int);

protected:
  vtk1DTransferFunctionChooser();
  ~vtk1DTransferFunctionChooser();

  int TransferFunctionMode;
  vtk1DLookupTableTransferFunction* LookupTableTransferFunction;
  vtk1DGaussianTransferFunction* GaussianTransferFunction;

private:
  vtk1DTransferFunctionChooser(const vtk1DTransferFunctionChooser&); // Not implemented.
  void operator=(const vtk1DTransferFunctionChooser&); // Not implemented.
};

#endif

