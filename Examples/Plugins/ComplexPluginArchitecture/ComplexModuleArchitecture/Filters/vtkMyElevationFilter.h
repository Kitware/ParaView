/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyElevationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vtkMyElevationFilter is a filter to generate scalar values from a
// dataset. It also print the value of Pi in radians.

#ifndef vtkMyElevationFilter_h
#define vtkMyElevationFilter_h

#include "FiltersModule.h"

#include <vtkElevationFilter.h>

class FILTERS_EXPORT vtkMyElevationFilter : public vtkElevationFilter
{
public:
  static vtkMyElevationFilter* New();
  vtkTypeMacro(vtkMyElevationFilter, vtkElevationFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMyElevationFilter() = default;
  ~vtkMyElevationFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMyElevationFilter(const vtkMyElevationFilter&) = delete;
  void operator=(const vtkMyElevationFilter&) = delete;
};

#endif
