// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMyOtherElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vtkMyOtherElevationFilter is a filter to generate scalar values from a
// dataset.

#ifndef vtkMyOtherElevationFilter_h
#define vtkMyOtherElevationFilter_h

#include "SharedModule.h" // for export macro

#include "vtkElevationFilter.h"

class SHARED_EXPORT vtkMyOtherElevationFilter : public vtkElevationFilter
{
public:
  static vtkMyOtherElevationFilter* New();
  vtkTypeMacro(vtkMyOtherElevationFilter, vtkElevationFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMyOtherElevationFilter() = default;
  ~vtkMyOtherElevationFilter() override = default;

private:
  vtkMyOtherElevationFilter(const vtkMyOtherElevationFilter&) = delete;
  void operator=(const vtkMyOtherElevationFilter&) = delete;
};

#endif
