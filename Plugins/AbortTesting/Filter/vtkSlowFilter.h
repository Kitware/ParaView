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

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSlowFilterModule.h" // For export macro

class VTKSLOWFILTER_EXPORT vtkSlowFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkSlowFilter* New();
  vtkTypeMacro(vtkSlowFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /*
   * Time to sleep between loops in ms
   * Default is 2
   */
  vtkSetMacro(SleepTime, int);
  vtkGetMacro(SleepTime, int);

private:
  vtkSlowFilter() = default;

  vtkSlowFilter(const vtkSlowFilter&) = delete;
  void operator=(const vtkSlowFilter&) = delete;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int SleepTime = 2;
};

#endif
