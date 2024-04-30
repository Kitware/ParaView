// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkLegacyStreaklineFilter is a filter that integrates a vector field to generate streak lines
 *
 *
 * @sa
 * vtkStreaklineFilter has the details of the algorithms
 */

#ifndef vtkLegacyStreaklineFilter_h
#define vtkLegacyStreaklineFilter_h

#include "vtkPVVTKExtensionsFiltersFlowPathsModule.h" // For export macro
#include "vtkStreaklineFilter.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkExtractTimeSteps;
class vtkStreaklineFilter;

class VTKPVVTKEXTENSIONSFILTERSFLOWPATHS_EXPORT vtkLegacyStreaklineFilter
  : public vtkStreaklineFilter
{
public:
  vtkTypeMacro(vtkLegacyStreaklineFilter, vtkStreaklineFilter);

  static vtkLegacyStreaklineFilter* New();

  double GetTerminationTime() VTK_FUTURE_CONST override { return this->TerminationTime; }
  void SetTerminationTime(double t) override
  {
    if (this->TerminationTime != t)
    {
      this->TerminationTime = t;
      this->Modified();
    }
  }

protected:
  vtkLegacyStreaklineFilter() = default;
  ~vtkLegacyStreaklineFilter() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double TerminationTime = 0;

private:
  vtkLegacyStreaklineFilter(const vtkLegacyStreaklineFilter&) = delete;
  void operator=(const vtkLegacyStreaklineFilter&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
