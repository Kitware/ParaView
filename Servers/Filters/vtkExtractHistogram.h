/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractHistogram.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkExtractHistogram_h
#define __vtkExtractHistogram_h

#include "vtkRectilinearGridAlgorithm.h"

// .NAME vtkExtractHistogram - Extract histogram data (binned values) from any dataset
// .SECTION Description
// vtkExtractHistogram accepts any vtkDataSet as input and produces a
// vtkPolyData containing histogram data as output.  The output vtkPolyData
// will have contain a vtkDoubleArray named "bin_extents" which contains
// the boundaries between each histogram bin, and a vtkUnsignedLongArray
// named "bin_values" which will contain the value for each bin.

class VTK_EXPORT vtkExtractHistogram : public vtkRectilinearGridAlgorithm
{
public:
  static vtkExtractHistogram* New();
  vtkTypeRevisionMacro(vtkExtractHistogram, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Controls which input data component should be binned, for input arrays
  // with more-than-one component
  vtkSetClampMacro(Component, int, 0, VTK_LARGE_INTEGER);
  vtkGetMacro(Component, int);
  
  // Description:
  // Controls the number of bins N in the output histogram data
  vtkSetClampMacro(BinCount, int, 1, VTK_LARGE_INTEGER);
  vtkGetMacro(BinCount, int);
  
private:
  vtkExtractHistogram();
  vtkExtractHistogram(const vtkExtractHistogram&); // Not implemented
  void operator=(const vtkExtractHistogram&); // Not implemented
  ~vtkExtractHistogram();

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


  int Component;
  int BinCount;
};

#endif
