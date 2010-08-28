/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPostFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPostFilter - Post Filter for on demand conversion
// .SECTION Description
// vtkPVPostFilter is a filter used for on demand conversion
// of properties
// Provide the ability to automatically use a vector component as a scalar
// input property.
//
//  Interpolate cell centered data to point data, and the inverse if needed
// by the filter.

#ifndef __vtkPVPostFilter_h
#define __vtkPVPostFilter_h

#include "vtkDataObjectAlgorithm.h"

class VTK_EXPORT vtkPVPostFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkPVPostFilter* New();
  vtkTypeMacro(vtkPVPostFilter,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // We need to override this method because the composite data pipeline
  // is not what we want. Instead we need the PVCompositeDataPipeline
  // so that we can figure out what we conversion(s) we need to do
  vtkExecutive* CreateDefaultExecutive();

protected:
  vtkPVPostFilter();
  ~vtkPVPostFilter();

  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int DetermineNeededConversion(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Determines if we are going to be converting cell to point or point to cell
  // also returns if we detected the property is a vectory property
  int DeterminePointCellConversion(vtkInformation *postArrayInfo, vtkDataObject *input);
  int DetermineVectorConversion(vtkInformation *postArrayInfo, vtkDataObject *input);

  bool MatchingPropertyInformation(vtkInformation* inputArrayInfo,vtkInformation* postArrayInfo);

private:
  vtkPVPostFilter(const vtkPVPostFilter&);  // Not implemented.
  void operator=(const vtkPVPostFilter&);  // Not implemented.
};

#endif