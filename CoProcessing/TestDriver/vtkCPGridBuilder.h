/*=========================================================================

  Program:   ParaView
  Module:    vtkCPGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  virtual vtkDataObject* GetGrid(
    unsigned long timeStep, double time, int& builtNewGrid) override = 0;

  //@{
  /**
   * Set/get the FieldBuilder.
   */
  void SetFieldBuilder(vtkCPBaseFieldBuilder* fieldBuilder);
  vtkCPBaseFieldBuilder* GetFieldBuilder();
  //@}

protected:
  vtkCPGridBuilder();
  ~vtkCPGridBuilder();

private:
  vtkCPGridBuilder(const vtkCPGridBuilder&) = delete;

  void operator=(const vtkCPGridBuilder&) = delete;
  //@{
  /**
   * The field builder for creating the input fields to the coprocessing
   * library.
   */
  vtkCPBaseFieldBuilder* FieldBuilder;
};
//@}

#endif
