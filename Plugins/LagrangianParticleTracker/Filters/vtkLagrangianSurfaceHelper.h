/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianSurfaceHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianSurfaceHelper
 * @brief   Filter to generate
 * surface data for a lagrangian integration model
 *
 *
 * This filter enables generation of field data array by array,
 * different for each block when using composite data,
 * from constant value
 */

#ifndef vtkLagrangianSurfaceHelper_h
#define vtkLagrangianSurfaceHelper_h

#include "vtkLagrangianHelperBase.h"
#include "vtkLagrangianParticleTrackerModule.h" // for export macro

class VTKLAGRANGIANPARTICLETRACKER_EXPORT vtkLagrangianSurfaceHelper
  : public vtkLagrangianHelperBase
{
public:
  static vtkLagrangianSurfaceHelper* New();
  vtkTypeMacro(vtkLagrangianSurfaceHelper, vtkLagrangianHelperBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Remove all arrays to generate, no more array will be generated
   */
  void RemoveAllArraysToGenerate() override;

  /**
   * Set the number of arrays to generate
   */
  void SetNumberOfArrayToGenerate(int i) override;

  /**
   * Set an array to generate
   */
  void SetArrayToGenerate(int i, const char* arrayName, int type, int numberOfLeafs,
    int numberOfComponents, const char* arrayValues) override;

protected:
  vtkLagrangianSurfaceHelper();
  ~vtkLagrangianSurfaceHelper() override;

  /**
   * Fill the model with inputs if any.
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Creates the same output type as the input type.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Convenience method to fill field data of provided dataset
   * using data for the specified leaf number
   */
  void FillFieldData(vtkDataSet* dataset, int leaf);

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkLagrangianSurfaceHelper(const vtkLagrangianSurfaceHelper&) = delete;
  void operator=(const vtkLagrangianSurfaceHelper&) = delete;
};

#endif
