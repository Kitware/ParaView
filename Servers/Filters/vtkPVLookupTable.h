/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookupTable.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLookupTable - a combination of vtkColorTransferFunction and
// vtkLookupTable.
// .SECTION Description
// This is a cross between a vtkColorTransferFunction and a vtkLookupTable
// selectively combiniting the functionality of both.
// NOTE: One must call Build() after making any changes to the points
// in the ColorTransferFunction to ensure that the discrete and non-discrete
// version match up.

#ifndef __vtkPVLookupTable_h
#define __vtkPVLookupTable_h

#include "vtkColorTransferFunction.h"

class vtkLookupTable;
class vtkColorTransferFunction;

class VTK_EXPORT vtkPVLookupTable : public vtkColorTransferFunction
{
public:
  static vtkPVLookupTable* New();
  vtkTypeRevisionMacro(vtkPVLookupTable, vtkColorTransferFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Generate discretized lookup table, if applicable.
  // This method must be called after changes to the ColorTransferFunction
  // otherwise the discretized version will be inconsitent with the 
  // non-discretized one.
  virtual void Build();

  // Description:
  // Set if the values are to mapped after discretization. The
  // number of discrete values is set by using SetNumberOfValues().
  // Not set by default, i.e. color value is determined by
  // interpolating at the scalar value.
  vtkSetMacro(Discretize, int);
  vtkGetMacro(Discretize, int);
  vtkBooleanMacro(Discretize, int);

  // Description:
  // Get/Set if log scale must be used while mapping scalars
  // to colors.
  vtkSetMacro(UseLogScale, int);
  vtkGetMacro(UseLogScale, int);

  // Description:
  // Set the number of values i.e. colors to be generated in the
  // discrete lookup table. This has no effect if Discretize is off.
  void SetNumberOfValues(vtkIdType number);
  vtkGetMacro(NumberOfValues, vtkIdType);

  // Description:
  // Map one value through the lookup table and return a color defined
  // as a RGBA unsigned char tuple (4 bytes).
  virtual unsigned char *MapValue(double v);

  // Description:
  // Map one value through the lookup table and return the color as
  // an RGB array of doubles between 0 and 1.
  virtual void GetColor(double v, double rgb[3]);

  // Description:
  // An internal method typically not used in applications.
  virtual void MapScalarsThroughTable2(void *input, unsigned char *output,
                                       int inputDataType, int numberOfValues,
                                       int inputIncrement, 
                                       int outputFormat);

  // Description:
  // Returns the (x, r, g, b) values as an array.
  double* GetRGBPoints();

protected:
  vtkPVLookupTable();
  ~vtkPVLookupTable();

  int Discretize;
  int UseLogScale;

  vtkIdType NumberOfValues;
  vtkLookupTable* LookupTable;

  vtkTimeStamp BuildTime;
private:
  vtkPVLookupTable(const vtkPVLookupTable&); // Not implemented.
  void operator=(const vtkPVLookupTable&); // Not implemented.

  // Pointer used by GetRGBPoints().
  double* Data;
};

#endif

