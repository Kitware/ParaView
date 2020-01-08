/*=========================================================================

  Program:   ParaView
  Module:    vtkCPBaseFieldBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCPBaseFieldBuilder
 * @brief   Abstract class for specifying fields over grids.
 *
 * Abstract class for specifying fields over grids for a test driver.
 * May want to remove GetHighestFieldOrder as it is just a place holder
 * for now.
*/

#ifndef vtkCPBaseFieldBuilder_h
#define vtkCPBaseFieldBuilder_h

#include "vtkObject.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class vtkDataSet;

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPBaseFieldBuilder : public vtkObject
{
public:
  vtkTypeMacro(vtkCPBaseFieldBuilder, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a grid.  BuiltNewField is set to 0 if the grids
   * that were returned were already built before.
   * vtkCPBaseFieldBuilder will also delete the grid.
   */
  virtual void BuildField(unsigned long TimeStep, double Time, vtkDataSet* Grid) = 0;

  /**
   * Return the highest order of discretization of the field.
   * virtual unsigned int GetHighestFieldOrder() = 0;
   */

protected:
  vtkCPBaseFieldBuilder();
  ~vtkCPBaseFieldBuilder();

private:
  vtkCPBaseFieldBuilder(const vtkCPBaseFieldBuilder&) = delete;
  void operator=(const vtkCPBaseFieldBuilder&) = delete;
};

#endif
