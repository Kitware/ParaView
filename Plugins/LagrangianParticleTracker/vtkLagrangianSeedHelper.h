/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianSeedHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianSeedHelper
 * @brief   Filter to generate
 * seed data for a lagrangian integration model
 *
 *
 * This filter enable to generate point data, array by array
 * from constant value or by interpolating from a volumic input
*/

#ifndef vtkLagrangianSeedHelper_h
#define vtkLagrangianSeedHelper_h

#include "vtkLagrangianHelperBase.h"

class vtkLagrangianSeedHelper : public vtkLagrangianHelperBase
{
public:
  static vtkLagrangianSeedHelper* New();
  vtkTypeMacro(vtkLagrangianSeedHelper, vtkLagrangianHelperBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum FlowOrConstant
  {
    FLOW = 0,
    CONSTANT = 1
  };

  /**
   * Set/Get the seed source input
   */
  void SetSourceData(vtkDataObject* source);

  /**
   * Set the seed source connection
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Remove all arrays to generate, no more array will be generated
   */
  void RemoveAllArraysToGenerate() VTK_OVERRIDE;

  /**
   * Set the number of arrays to generate
   */
  void SetNumberOfArrayToGenerate(int i) VTK_OVERRIDE;

  /**
   * Add an array to generate, eiher from a constant value or by interpolation from the flow
   */
  void SetArrayToGenerate(int i, const char* arrayName, int type, int flowOrConstant,
    int numberOfComponents, const char* arrayValues) VTK_OVERRIDE;

protected:
  vtkLagrangianSeedHelper();
  ~vtkLagrangianSeedHelper();

  /**
   * Creates the same output type as the input type for non composite dataset
   * In case of composite, the output will have the same type of the first block
   * of the input
   */
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkLagrangianSeedHelper(const vtkLagrangianSeedHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLagrangianSeedHelper&) VTK_DELETE_FUNCTION;
};

#endif
