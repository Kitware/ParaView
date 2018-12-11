/*=========================================================================

  Program:   ParaView
  Module:    vtkCPNodalFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  virtual void BuildField(unsigned long timeStep, double time, vtkDataSet* grid) override;

protected:
  vtkCPNodalFieldBuilder();
  ~vtkCPNodalFieldBuilder();

private:
  vtkCPNodalFieldBuilder(const vtkCPNodalFieldBuilder&) = delete;
  void operator=(const vtkCPNodalFieldBuilder&) = delete;
};

#endif
