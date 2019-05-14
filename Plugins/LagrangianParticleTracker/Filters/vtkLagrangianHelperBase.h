/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianHelperBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianHelperBase
 * @brief   Abstract class for lagrangian helper
 *
 *
 * This is an abstract class for Lagrangian helper
 * It defines the integration model
 * as well as the SetArrayToGenerate method signature
 */

#ifndef vtkLagrangianHelperBase_h
#define vtkLagrangianHelperBase_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkLagrangianParticleTrackerModule.h" // for export macro

class vtkLagrangianBasicIntegrationModel;
class VTKLAGRANGIANPARTICLETRACKER_EXPORT vtkLagrangianHelperBase : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkLagrangianHelperBase, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the integration model.
   */
  void SetIntegrationModel(vtkLagrangianBasicIntegrationModel* integrationModel);
  vtkGetObjectMacro(IntegrationModel, vtkLagrangianBasicIntegrationModel);
  //@}

  /**
   * Remove all arrays to generate, no more array will be generated
   */
  virtual void RemoveAllArraysToGenerate() = 0;

  /**
   * Set the number of array to generate
   */
  virtual void SetNumberOfArrayToGenerate(int i) = 0;

  /**
   * Add an array to generate, eiher from a constant value or from a file
   */
  virtual void SetArrayToGenerate(int i, const char* arrayName, int type, int firstInt,
    int secondInt, const char* arrayValues) = 0;

protected:
  vtkLagrangianHelperBase();
  ~vtkLagrangianHelperBase() override;

  /**
   * Parse string array and extract double components from it.
   */
  bool ParseDoubleValues(const char*& arrayString, int numberOfComponents, double* array);

  vtkLagrangianBasicIntegrationModel* IntegrationModel;

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkLagrangianHelperBase(const vtkLagrangianHelperBase&) = delete;
  void operator=(const vtkLagrangianHelperBase&) = delete;
};

#endif
