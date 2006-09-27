/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReductionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReductionFilter - A generic filter that can reduce any type of
// dataset using any reduction algorithm.
// .SECTION Description
// A generic filter that can reduce any type of dataset using any reduction 
// algorithm. Actual reduction is performed by ReductionHelper. ReductionHelper
// is an algorithm that takes multiple input connections and 
// produces a single reduced output.

#ifndef __vtkReductionFilter_h
#define __vtkReductionFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;

class VTK_EXPORT vtkReductionFilter : public vtkDataSetAlgorithm
{
public:
  static vtkReductionFilter* New();
  vtkTypeRevisionMacro(vtkReductionFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the reduction helper. Reduction helper is an algorithm with
  // multiple input connections, that produces a single output as
  // the reduced output.
  void SetReductionHelper(vtkAlgorithm*);
  vtkGetObjectMacro(ReductionHelper, vtkAlgorithm);

  // Description:
  // Get/Set the MPI controller used for gathering.
  void SetController(vtkMultiProcessController*);
protected:
  vtkReductionFilter();
  ~vtkReductionFilter();

  // Overridden to mark input as optional, since input data may
  // not be available on all processes that this filter is instantiated.
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  void MarshallData(vtkDataSet* input);
  vtkDataSet* Reconstruct(char* raw_data, int data_length);
  void Reduce(vtkDataSet* input, vtkDataSet* output);

  char* RawData;
  int DataLength;
  vtkAlgorithm* ReductionHelper;
  vtkMultiProcessController* Controller;
private:
  vtkReductionFilter(const vtkReductionFilter&); // Not implemented.
  void operator=(const vtkReductionFilter&); // Not implemented.
};

#endif

