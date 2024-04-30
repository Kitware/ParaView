// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkLegacyParticlePathFilter is a filter that integrates a vector field to generate streak lines
 *
 *
 * @sa
 * vtkParticlePathFilter has the details of the algorithms
 */

#ifndef vtkLegacyParticlePathFilter_h
#define vtkLegacyParticlePathFilter_h

#include "vtkPVVTKExtensionsFiltersFlowPathsModule.h" // For export macro
#include "vtkParticlePathFilter.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkExtractTimeSteps;
class vtkParticlePathFilter;

class VTKPVVTKEXTENSIONSFILTERSFLOWPATHS_EXPORT vtkLegacyParticlePathFilter
  : public vtkParticlePathFilter
{
public:
  vtkTypeMacro(vtkLegacyParticlePathFilter, vtkParticlePathFilter);

  static vtkLegacyParticlePathFilter* New();

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
  vtkLegacyParticlePathFilter() = default;
  ~vtkLegacyParticlePathFilter() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double TerminationTime = 0;

private:
  vtkLegacyParticlePathFilter(const vtkLegacyParticlePathFilter&) = delete;
  void operator=(const vtkLegacyParticlePathFilter&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
