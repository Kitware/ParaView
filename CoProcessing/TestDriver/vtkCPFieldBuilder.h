/*=========================================================================

  Program:   ParaView
  Module:    vtkCPFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCPFieldBuilder
 * @brief   Abstract class for specifying fields over grids.
 *
 * Abstract class for specifying fields over grids for a test driver.
*/

#ifndef vtkCPFieldBuilder_h
#define vtkCPFieldBuilder_h

#include "vtkCPBaseFieldBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class vtkCPTensorFieldFunction;

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPFieldBuilder : public vtkCPBaseFieldBuilder
{
public:
  vtkTypeMacro(vtkCPFieldBuilder, vtkCPBaseFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a field on Grid.
   */
  virtual void BuildField(unsigned long TimeStep, double Time, vtkDataSet* Grid) override = 0;

  /**
   * Return the highest order of discretization of the field.
   * virtual unsigned int GetHighestFieldOrder() = 0;
   */

  //@{
  /**
   * Set/get the name of the field array.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Set/get TensorFieldFunction.
   */
  void SetTensorFieldFunction(vtkCPTensorFieldFunction* TFF);
  vtkCPTensorFieldFunction* GetTensorFieldFunction();
  //@}

protected:
  vtkCPFieldBuilder();
  ~vtkCPFieldBuilder();

private:
  vtkCPFieldBuilder(const vtkCPFieldBuilder&) = delete;
  void operator=(const vtkCPFieldBuilder&) = delete;

  /**
   * The name of the array that will be inserted into the point/cell data.
   */
  char* ArrayName;

  //@{
  /**
   * The function that actually computes the tensor field values at
   * specified points.
   */
  vtkCPTensorFieldFunction* TensorFieldFunction;
};
//@}

#endif
