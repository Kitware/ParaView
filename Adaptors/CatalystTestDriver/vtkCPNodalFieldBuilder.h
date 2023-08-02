// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCPNodalFieldBuilder
 * @brief   Class for specifying nodal fields over grids.
 *
 * Class for specifying nodal fields over grids for a test driver.
 */

#ifndef vtkCPNodalFieldBuilder_h
#define vtkCPNodalFieldBuilder_h

#include "vtkCPFieldBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPNodalFieldBuilder : public vtkCPFieldBuilder
{
public:
  static vtkCPNodalFieldBuilder* New();
  vtkTypeMacro(vtkCPNodalFieldBuilder, vtkCPFieldBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a field on Grid.
   */
  void BuildField(unsigned long timeStep, double time, vtkDataSet* grid) override;

protected:
  vtkCPNodalFieldBuilder();
  ~vtkCPNodalFieldBuilder() override;

private:
  vtkCPNodalFieldBuilder(const vtkCPNodalFieldBuilder&) = delete;
  void operator=(const vtkCPNodalFieldBuilder&) = delete;
};

#endif
