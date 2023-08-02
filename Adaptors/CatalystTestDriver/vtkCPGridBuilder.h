// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPGridBuilder
 * @brief   Abstract class for creating grids.
 *
 * Abstract class for creating grids for a test driver.
 */

#ifndef vtkCPGridBuilder_h
#define vtkCPGridBuilder_h

#include "vtkCPBaseGridBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class vtkDataObject;
class vtkCPBaseFieldBuilder;

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPGridBuilder : public vtkCPBaseGridBuilder
{
public:
  vtkTypeMacro(vtkCPGridBuilder, vtkCPBaseGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a grid.  BuiltNewGrid is set to 0 if the grids
   * that were returned were already built before.
   * vtkCPGridBuilder will also delete the grid.
   */
  vtkDataObject* GetGrid(unsigned long timeStep, double time, int& builtNewGrid) override = 0;

  ///@{
  /**
   * Set/get the FieldBuilder.
   */
  void SetFieldBuilder(vtkCPBaseFieldBuilder* fieldBuilder);
  vtkCPBaseFieldBuilder* GetFieldBuilder();
  ///@}

protected:
  vtkCPGridBuilder();
  ~vtkCPGridBuilder() override;

private:
  vtkCPGridBuilder(const vtkCPGridBuilder&) = delete;

  void operator=(const vtkCPGridBuilder&) = delete;
  ///@{
  /**
   * The field builder for creating the input fields to the coprocessing
   * library.
   */
  vtkCPBaseFieldBuilder* FieldBuilder;
};
//@}

#endif
