// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyPParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * vtkLegacyPParticlePathFilter is a filter that integrates a vector field to generate
 * path lines.
 *
 * @sa
 * vtkLegacyPParticlePathFilterBase has the details of the algorithms
 */

#ifndef vtkLegacyPParticlePathFilter_h
#define vtkLegacyPParticlePathFilter_h

#include "vtkLegacyPParticleTracerBase.h"
#include "vtkPVVTKExtensionsFiltersGeneralMPIModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkPolyData;

class VTKPVVTKEXTENSIONSFILTERSGENERALMPI_EXPORT vtkLegacyPParticlePathFilter
  : public vtkLegacyPParticleTracerBase
{
public:
  vtkTypeMacro(vtkLegacyPParticlePathFilter, vtkLegacyPParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkLegacyPParticlePathFilter* New();

protected:
  vtkLegacyPParticlePathFilter();
  ~vtkLegacyPParticlePathFilter() override;

  void ResetCache() override;
  int OutputParticles(vtkPolyData* poly) override;
  void InitializeExtraPointDataArrays(vtkPointData* outputPD) override;
  void SetToExtraPointDataArrays(
    vtkIdType particleId, vtkLegacyParticleTracerBaseNamespace::ParticleInformation&) override;
  void Finalize() override;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  class ParticlePathFilterInternal
  {
  public:
    ParticlePathFilterInternal()
      : Filter(nullptr)
    {
    }
    void Initialize(vtkLegacyParticleTracerBase* filter);
    virtual ~ParticlePathFilterInternal() = default;
    virtual int OutputParticles(vtkPolyData* particles);
    void SetClearCache(bool clearCache) { this->ClearCache = clearCache; }
    bool GetClearCache() { return this->ClearCache; }
    void Finalize();
    void Reset();

  private:
    vtkLegacyParticleTracerBase* Filter;
    // Paths doesn't seem to work properly. it is meant to make connecting lines
    // for the particles
    std::vector<vtkSmartPointer<vtkIdList>> Paths;
    bool ClearCache; // false by default
  };

  ParticlePathFilterInternal It;
  vtkDoubleArray* SimulationTime;
  vtkIntArray* SimulationTimeStep;

private:
  vtkLegacyPParticlePathFilter(const vtkLegacyPParticlePathFilter&) = delete;
  void operator=(const vtkLegacyPParticlePathFilter&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
