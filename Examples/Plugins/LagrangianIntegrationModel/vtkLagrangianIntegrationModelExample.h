/*=========================================================================

  Program:   ParaView
  Module:    vtkLagrangianIntegrationModelExample.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianIntegrationModelExample
 * @brief   Integration model example
 *
 * Similar to vtkLagrangianMatidaIntegrationModel
 * this integration model demonstrates the capabilities
 * of Lagrangian Integration Models
 * The mains differences with Matida version are
 * it uses a gravity constant from the data, instead of using G,
 * and it also adds new variable, the particle diameter,
 * and uses this diameter to make choice during interaction and free flight
 * Please consult vtkLagrangianBasicIntegrationModel and vtkLagrangianMatidaIntegrationModel
 * for more explanation on how this example works.
*/

#ifndef vtkLagrangianIntegrationModelExample_h
#define vtkLagrangianIntegrationModelExample_h

#include "vtkLagrangianBasicIntegrationModel.h"

class vtkLagrangianIntegrationModelExample : public vtkLagrangianBasicIntegrationModel
{
public:
  vtkTypeMacro(vtkLagrangianIntegrationModelExample, vtkLagrangianBasicIntegrationModel);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkLagrangianIntegrationModelExample* New();

  using Superclass::FunctionValues;

  /**
   * Evaluate the integration model velocity field
   * f at position x, causing data from cell in dataSet with index cellId
   * This method is pure abstract at vtkLagrangianBasicIntegrationModel
   * THIS IS THE MAIN METHOD TO BE DEFINED IN A LAGRANGIAN INTEGRATION MODEL PLUGIN
   */
  virtual int FunctionValues(
    vtkDataSet* dataSet, vtkIdType cellId, double* weights, double* x, double* f) VTK_OVERRIDE;

  /**
   * This method is called each time a particle created from the seeds
   * It should be inherited in order to initialize variable data in user variables
   * from seed data
   */
  virtual void InitializeParticle(vtkLagrangianParticle* particle) VTK_OVERRIDE;

  /**
   * This method is called when initializing output point data
   * It should be inherited when there is some variables data needed to be put
   * in output field data.
   * Add some User Variable Data Array in provided particleData, allocate
   * maxTuples tuples.
   */
  virtual void InitializeVariablesParticleData(
    vtkPointData* particleData, int maxTuples = 0) VTK_OVERRIDE;

  /**
   * This method is called when inserting particle data in output point data
   * It should be inherited when there is some variables data needed to be put in
   * output field data.
   * Insert user variables data in provided point data, user variables data array begins at
   * arrayOffset. use stepEnum to identify which step ( prev, current or next ) should be inserted.
   */
  virtual void InsertVariablesParticleData(
    vtkLagrangianParticle* particle, vtkPointData* data, int stepEnum) VTK_OVERRIDE;

  /**
   * This method is called when checking if a particle should be terminated in free flight
   * At vtkLagrangianBasicIntegrationModel this method does nothing
   * Return true if particle is terminated, false otherwise
   */
  virtual bool CheckFreeFlightTermination(vtkLagrangianParticle* particle) VTK_OVERRIDE;

  /**
   * Methods used by ParaView surface helper to get default
   * values for each leaf of each dataset of surface
   * nComponents could be retrived with arrayName but is
   * given for simplication purposes.
   * it is your responsability to initialize all components of
   * defaultValues[nComponent]
   */
  virtual void ComputeSurfaceDefaultValues(const char* arrayName, vtkDataSet* dataset,
    int nComponents, double* defaultValues) VTK_OVERRIDE;

protected:
  vtkLagrangianIntegrationModelExample();
  virtual ~vtkLagrangianIntegrationModelExample();

  /**
   * This method is called each time a particle interact with a surface
   * With an unrecodgnized surfaceType or SURFACE_TYPE_MODEL
   * The particle next position is already positioned exactly on the surface and
   * position of the particle is not suposed to be changed
   * It is possible in this method to choose to terminate particle, alter it's variables including
   * velocity,
   * create new particle...
   */
  virtual bool InteractWithSurface(int surfaceType, vtkLagrangianParticle* particle,
    vtkDataSet* surface, vtkIdType cellId,
    std::queue<vtkLagrangianParticle*>& particles) VTK_OVERRIDE;

  /**
   * This method is called when trying to find the intersection point between a particle
   * and a surface, enabling to use your any intersection code. in this case it only call the
   * superclass method
   */
  virtual bool IntersectWithLine(
    vtkCell* cell, double p1[3], double p2[3], double tol, double& t, double x[3]) VTK_OVERRIDE;

  double GetRelaxationTime(const double& dynVisc, const double& diameter, const double& density);

  double GetDragCoefficient(const double* flowVelocity, const double* particleVelocity,
    const double& dynVisc, const double& particleDiameter, const double& flowDensity);

private:
  vtkLagrangianIntegrationModelExample(
    const vtkLagrangianIntegrationModelExample&);              // Not implemented.
  void operator=(const vtkLagrangianIntegrationModelExample&); // Not implemented.
};

#endif
