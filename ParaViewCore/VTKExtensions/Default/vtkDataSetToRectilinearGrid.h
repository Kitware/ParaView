/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetToRectilinearGrid - Extract histogram data (binned values) from any 
// dataset
// .SECTION Description
// vtkDataSetToRectilinearGrid accepts any vtkDataSet as input and produces a
// vtkPolyData containing histogram data as output.  The output vtkPolyData
// will have contain a vtkDoubleArray named "bin_extents" which contains
// the boundaries between each histogram bin, and a vtkUnsignedLongArray
// named "bin_values" which will contain the value for each bin.


#ifndef __vtkDataSetToRectilinearGrid_h
#define __vtkDataSetToRectilinearGrid_h

#include "vtkRectilinearGridAlgorithm.h"

class VTK_EXPORT vtkDataSetToRectilinearGrid : public vtkRectilinearGridAlgorithm
{
public:
  static vtkDataSetToRectilinearGrid* New();
  vtkTypeMacro(vtkDataSetToRectilinearGrid, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected: 
  vtkDataSetToRectilinearGrid();
  ~vtkDataSetToRectilinearGrid();


  virtual int FillInputPortInformation (int port, vtkInformation *info);

  // convenience method
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation *request, 
                          vtkInformationVector **inputVector, 
                          vtkInformationVector *outputVector);

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

private:
  void operator=(const vtkDataSetToRectilinearGrid&); // Not implemented
  vtkDataSetToRectilinearGrid(const vtkDataSetToRectilinearGrid&); // Not implemented
};

#endif
