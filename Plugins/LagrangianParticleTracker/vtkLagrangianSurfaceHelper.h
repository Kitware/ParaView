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

class vtkLagrangianSurfaceHelper : public vtkLagrangianHelperBase
{
public:
  static vtkLagrangianSurfaceHelper* New();
  vtkTypeMacro(vtkLagrangianSurfaceHelper, vtkLagrangianHelperBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Remove all arrays to generate, no more array will be generated
   */
  void RemoveAllArraysToGenerate() VTK_OVERRIDE;

  /**
   * Set the number of arrays to generate
   */
  void SetNumberOfArrayToGenerate(int i) VTK_OVERRIDE;

  /**
   * Set an array to generate
   */
  void SetArrayToGenerate(int i, const char* arrayName, int type, int numberOfLeafs,
    int numberOfComponents, const char* arrayValues) VTK_OVERRIDE;

protected:
  vtkLagrangianSurfaceHelper();
  ~vtkLagrangianSurfaceHelper();

  /**
   * Fill the model with inputs if any.
   */
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  /**
   * Creates the same output type as the input type.
   */
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Convenience method to fill field data of provided dataset
   * using data for the speicfied leaf number
   */
  void FillFieldData(vtkDataSet* dataset, int leaf);

  class vtkInternals;
  vtkInternals* Internals;

private:
  vtkLagrangianSurfaceHelper(const vtkLagrangianSurfaceHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLagrangianSurfaceHelper&) VTK_DELETE_FUNCTION;
};

#endif
