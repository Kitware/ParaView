// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMyElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vtkMyElevationFilter is a filter to generate scalar values from a
// dataset. It also print the value of Pi in radians.

#ifndef vtkMyElevationFilter_h
#define vtkMyElevationFilter_h

#include "FiltersModule.h" // for export macro

#include "vtkElevationFilter.h"

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
