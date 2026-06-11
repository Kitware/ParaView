// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSlowFilter
 * @brief   Slow filter to invoke ProgressEvent
 *
 * A simple filter to trigger ProgressEvent
 */

#ifndef vtkSlowFilter_h
#define vtkSlowFilter_h

#include "vtkSlowFilterModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKSLOWFILTER_EXPORT vtkSlowFilter : public vtkStructuredGridAlgorithm
{
public:
  static vtkSlowFilter* New();
  vtkTypeMacro(vtkSlowFilter, vtkStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

private:
  vtkSlowFilter() = default;

  vtkSlowFilter(const vtkSlowFilter&) = delete;
  void operator=(const vtkSlowFilter&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
};

VTK_ABI_NAMESPACE_END
#endif
