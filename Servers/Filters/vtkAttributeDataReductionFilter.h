/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataReductionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAttributeDataReductionFilter - Reduces cell/point attribute data 
// with different modes to combine cell/point data.
// .SECTION Description
// Filter that takes data with same structure on multiple input connections to
// produce a reduced dataset with cell/point data summed/maxed/minned for 
// all cells/points. Data arrays not available in all inputs
// are discarded.

#ifndef __vtkAttributeDataReductionFilter_h
#define __vtkAttributeDataReductionFilter_h

#include "vtkDataSetAlgorithm.h"

class VTK_EXPORT vtkAttributeDataReductionFilter : public vtkDataSetAlgorithm
{
public:
  static vtkAttributeDataReductionFilter* New();
  vtkTypeRevisionMacro(vtkAttributeDataReductionFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
//BTX
  enum ReductionTypes
    {
    ADD = 1,
    MAX = 2,
    MIN = 3
    };
//ETX

  // Set the reduction type. Reduction type dictates how overlapping cell/point
  // data is combined. Default is ADD.
  vtkSetMacro(ReductionType, int);
  vtkGetMacro(ReductionType, int);
  void SetReductionTypeToAdd() 
    { this->SetReductionType(vtkAttributeDataReductionFilter::ADD); }
  void SetReductionTypeToMax() 
    { this->SetReductionType(vtkAttributeDataReductionFilter::MAX); }
  void SetReductionTypeToMin() 
    { this->SetReductionType(vtkAttributeDataReductionFilter::MIN); }
protected:
  vtkAttributeDataReductionFilter();
  ~vtkAttributeDataReductionFilter();
  
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  
  int ReductionType;
private:
  vtkAttributeDataReductionFilter(const vtkAttributeDataReductionFilter&); // Not implemented.
  void operator=(const vtkAttributeDataReductionFilter&); // Not implemented.
};

#endif
