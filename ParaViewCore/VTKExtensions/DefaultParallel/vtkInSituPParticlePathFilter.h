/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInSituPParticlePathFilter
 * @brief   An In Situ Particle tracer for unsteady vector fields
 *
 * vtkInSituPParticlePathFilter is a filter that integrates a vector field
 * to generate particle paths. It is intended for in situ use. The additions
 * to the parallel particle path filter is that the particle locations
 * at previous time steps can be cleared out (ClearCache data member)
 * and restarted connection can be used to continue advecting particles
 * from a restarted simulation.
 * @sa
 * vtkPParticlePathFilterBase has the details of the algorithms
*/

#ifndef vtkInSituPParticlePathFilter_h
#define vtkInSituPParticlePathFilter_h

#include "vtkPParticlePathFilter.h"
#include "vtkPVVTKExtensionsDefaultParallelModule.h" //needed for exports

class VTKPVVTKEXTENSIONSDEFAULTPARALLEL_EXPORT vtkInSituPParticlePathFilter
  : public vtkPParticlePathFilter
{
public:
  vtkTypeMacro(vtkInSituPParticlePathFilter, vtkPParticlePathFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkInSituPParticlePathFilter* New();

  /**
   * Set whether or not to clear out cache of previous time steps.
   * Default value is false. Clearing the cache is aimed towards in situ use.
   */
  void SetClearCache(bool);

  //@{
  /**
   * Provide support pre-pending previously computed particle paths.
   */
  void AddRestartConnection(vtkAlgorithmOutput* input);
  void RemoveAllRestarts();
  //@}

  //@{
  /**
   * Set/Get if the simulation has been restarted. If it is a restarted
   * simulation we may not necessarily want to inject/reinject particles
   * on the first time step. The default is False.
   */
  vtkSetMacro(RestartedSimulation, bool);
  vtkGetMacro(RestartedSimulation, bool);
  //@}

  //@{
  /**
   * Specify the first time step that particle paths are computed.
   * This is useful for restarted simulations or when the simulation's
   * first time step isn't 0. The default is 0.
   */
  vtkSetMacro(FirstTimeStep, int);
  vtkGetMacro(FirstTimeStep, int);
  //@}

protected:
  vtkInSituPParticlePathFilter();
  ~vtkInSituPParticlePathFilter();

  /**
   * Overriding this method allows us to inject the seeds as some
   * point other than the first time step. This is needed for
   * restarted simulations where the restart step may not
   * coincide with an injection step. Even if it did, the seeds
   * probably were already reinjected before the restart files
   * were written.
   */
  virtual std::vector<vtkDataSet*> GetSeedSources(
    vtkInformationVector* inputVector, int timeStep) override;

  /**
   * We add in a third, optional port for adding in particles for a
   * restarted simulation. These particles are only added at the first
   * time step.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * For restarts of particle paths, we add in the ability to add in
   * particles from a previous computation that we will still advect.
   */
  virtual void AddRestartSeeds(vtkInformationVector**) override;

  /**
   * Before starting the particle trace, classify
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   * This takes into account that the "seeds" may just be restarted
   * particles.
   */
  virtual void AssignSeedsToProcessors(double time, vtkDataSet* source, int sourceID, int ptId,
    vtkParticleTracerBaseNamespace::ParticleVector& localSeedPoints,
    int& localAssignedCount) override;

private:
  vtkInSituPParticlePathFilter(const vtkInSituPParticlePathFilter&) = delete;
  void operator=(const vtkInSituPParticlePathFilter&) = delete;

  /**
   * For restart input we need to mark that we will use arrays to
   * create new particles so that we can get proper history information
   * from them.
   */
  bool UseArrays;

  /**
   * Specify whether or not the particle paths are computed for a
   * restarted simulation. The default is False indicating this
   * is not a restarted simulation.
   */
  bool RestartedSimulation;

  //@{
  /**
   * Specify the first simulation time step that particles are computed.
   * This is useful for restarted simulations as well as simulations or
   * particle path tracking that don't start at time step 0. This is
   * used in computing the time step age and injected step id in
   * the output.
   */
  int FirstTimeStep;
};
#endif
//@}
