/*=========================================================================

  Program:   ParaView
  Module:    vtkCPCellFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  virtual void BuildField(unsigned long TimeStep, double Time, vtkDataSet* Grid) override;

  /**
   * Return the highest order of discretization of the field.
   * virtual unsigned int GetHighestFieldOrder();
   */

protected:
  vtkCPCellFieldBuilder();
  ~vtkCPCellFieldBuilder();

private:
  vtkCPCellFieldBuilder(const vtkCPCellFieldBuilder&) = delete;
  void operator=(const vtkCPCellFieldBuilder&) = delete;
};

#endif
