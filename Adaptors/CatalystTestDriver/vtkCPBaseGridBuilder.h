// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPBaseGridBuilder
 * @brief   Abstract class for creating grids.
 *
 * Abstract class for creating grids for a test driver.
 */

#ifndef vtkCPBaseGridBuilder_h
#define vtkCPBaseGridBuilder_h

#include "vtkObject.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class vtkDataObject;

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPBaseGridBuilder : public vtkObject
{
public:
  vtkTypeMacro(vtkCPBaseGridBuilder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a grid.  BuiltNewGrid is 0 if the grid is the same
   * as the last time step.
   */
  virtual vtkDataObject* GetGrid(unsigned long TimeStep, double Time, int& BuiltNewGrid) = 0;

  // maybe also have a subdivide grid cells here as well

protected:
  vtkCPBaseGridBuilder();
  ~vtkCPBaseGridBuilder() override;

private:
  vtkCPBaseGridBuilder(const vtkCPBaseGridBuilder&) = delete;
  void operator=(const vtkCPBaseGridBuilder&) = delete;
};

#endif
