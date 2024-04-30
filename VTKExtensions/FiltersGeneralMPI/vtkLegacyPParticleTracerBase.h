// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticleTracerBase
 * @brief   A parallel particle tracer for vector fields
 *
 * vtkLegacyPParticleTracerBase is the base class for parallel filters that advect particles
 * in a vector field. Note that the input vtkPointData structure must
 * be identical on all datasets.
 * @sa
 * vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkStreamTracer
 */

#ifndef vtkLegacyPParticleTracerBase_h
#define vtkLegacyPParticleTracerBase_h

#include "vtkLegacyParticleTracerBase.h"
#include "vtkSmartPointer.h" // For protected ivars.

#include <vector> // STL Header

#include "vtkPVVTKExtensionsFiltersGeneralMPIModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKPVVTKEXTENSIONSFILTERSGENERALMPI_EXPORT vtkLegacyPParticleTracerBase
  : public vtkLegacyParticleTracerBase
{
public:
  vtkTypeMacro(vtkLegacyPParticleTracerBase, vtkLegacyParticleTracerBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the controller used when sending particles between processes
   * The controller must be an instance of vtkMPIController.
   */
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  struct RemoteParticleInfo
  {
    vtkLegacyParticleTracerBaseNamespace::ParticleInformation Current;
    vtkLegacyParticleTracerBaseNamespace::ParticleInformation Previous;
    vtkSmartPointer<vtkPointData> PreviousPD;
  };

  typedef std::vector<RemoteParticleInfo> RemoteParticleVector;

  vtkLegacyPParticleTracerBase();
  ~vtkLegacyPParticleTracerBase() override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  //

  vtkPolyData* Execute(vtkInformationVector** inputVector) override;
  bool SendParticleToAnotherProcess(vtkLegacyParticleTracerBaseNamespace::ParticleInformation& info,
    vtkLegacyParticleTracerBaseNamespace::ParticleInformation& previous, vtkPointData*) override;

  /**
   * Before starting the particle trace, classify
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  void AssignSeedsToProcessors(double time, vtkDataSet* source, int sourceID, int ptId,
    vtkLegacyParticleTracerBaseNamespace::ParticleVector& localSeedPoints,
    int& localAssignedCount) override;

  /**
   * give each one a unique ID. We need to use MPI to find out
   * who is using which numbers.
   */
  void AssignUniqueIds(
    vtkLegacyParticleTracerBaseNamespace::ParticleVector& localSeedPoints) override;

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain. Returns
   * true if particles were migrated to any new process.
   */
  bool SendReceiveParticles(RemoteParticleVector& outofdomain, RemoteParticleVector& received);

  bool UpdateParticleListFromOtherProcesses() override;

  /**
   * Method that checks that the input arrays are ordered the
   * same on all data sets. This needs to be true for all
   * blocks in a composite data set as well as across all processes.
   */
  bool IsPointDataValid(vtkDataObject* input) override;

  //

  //

  // MPI controller needed when running in parallel
  vtkMultiProcessController* Controller;

  // List used for transmitting between processors during parallel operation
  RemoteParticleVector MPISendList;

  RemoteParticleVector Tail; // this is to receive the "tails" of traces from other processes
private:
  vtkLegacyPParticleTracerBase(const vtkLegacyPParticleTracerBase&) = delete;
  void operator=(const vtkLegacyPParticleTracerBase&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
