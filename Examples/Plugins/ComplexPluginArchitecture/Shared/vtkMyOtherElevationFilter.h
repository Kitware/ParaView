/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMyOtherElevationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMyOtherElevationFilter - generate scalars along a specified direction
// .SECTION Description
// vtkMyOtherElevationFilter is a filter to generate scalar values from a
// dataset.

#ifndef vtkMyOtherElevationFilter_h
#define vtkMyOtherElevationFilter_h

#include "SharedModule.h"

#include <vtkElevationFilter.h>

class SHARED_EXPORT vtkMyOtherElevationFilter : public vtkElevationFilter
{
public:
  static vtkMyOtherElevationFilter* New();
  vtkTypeMacro(vtkMyOtherElevationFilter, vtkElevationFilter);

protected:
  vtkMyOtherElevationFilter() = default;
  ~vtkMyOtherElevationFilter() override = default;

private:
  vtkMyOtherElevationFilter(const vtkMyOtherElevationFilter&) = delete;
  void operator=(const vtkMyOtherElevationFilter&) = delete;
};

#endif
