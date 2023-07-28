// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPCellFieldBuilder
 * @brief   Class for specifying cell fields over grids.
 *
 * Class for specifying cell data fields over grids for a test driver.
 */

#ifndef vtkCPCellFieldBuilder_h
#define vtkCPCellFieldBuilder_h

#include "vtkCPFieldBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPCellFieldBuilder : public vtkCPFieldBuilder
{
public:
  static vtkCPCellFieldBuilder* New();
  vtkTypeMacro(vtkCPCellFieldBuilder, vtkCPFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a field on Grid.
   */
  void BuildField(unsigned long TimeStep, double Time, vtkDataSet* Grid) override;

  /**
   * Return the highest order of discretization of the field.
   * virtual unsigned int GetHighestFieldOrder();
   */

protected:
  vtkCPCellFieldBuilder();
  ~vtkCPCellFieldBuilder() override;

private:
  vtkCPCellFieldBuilder(const vtkCPCellFieldBuilder&) = delete;
  void operator=(const vtkCPCellFieldBuilder&) = delete;
};

#endif
