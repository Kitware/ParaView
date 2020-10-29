/*=========================================================================

  Program:   ParaView
  Module:    vtkCPUniformGridBuilder.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCPUniformGridBuilder
 * @brief   Class for creating uniform grids.
 *
 * Class for creating vtkUniformGrids for a test driver.
*/

#ifndef vtkCPUniformGridBuilder_h
#define vtkCPUniformGridBuilder_h

#include "vtkCPGridBuilder.h"
#include "vtkPVCatalystTestDriverModule.h" // needed for export macros

class vtkDataObject;
class vtkUniformGrid;

class VTKPVCATALYSTTESTDRIVER_EXPORT vtkCPUniformGridBuilder : public vtkCPGridBuilder
{
public:
  static vtkCPUniformGridBuilder* New();
  vtkTypeMacro(vtkCPUniformGridBuilder, vtkCPGridBuilder);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return a grid.  BuiltNewGrid is set to 0 if the grids
   * that were returned were already built before.
   * vtkCPUniformGridBuilder will also delete the grid.
   */
  virtual vtkDataObject* GetGrid(unsigned long timeStep, double time, int& builtNewGrid) override;

  //@{
  /**
   * Set/get the Dimensions of the uniform grid.
   */
  vtkSetVector3Macro(Dimensions, int);
  int* GetDimensions();
  //@}

  //@{
  /**
   * Set/get the Dimensions of the uniform grid.
   */
  vtkSetVector3Macro(Spacing, double);
  double* GetSpacing();
  //@}

  //@{
  /**
   * Set/get the Dimensions of the uniform grid.
   */
  vtkSetVector3Macro(Origin, double);
  double* GetOrigin();
  //@}

  /**
   * Get the UniformGrid.
   */
  vtkUniformGrid* GetUniformGrid();

  /**
   * Create UniformGrid with the current parameters.  Returns true if
   * a new grid was created and false otherwise.
   */
  bool CreateUniformGrid();

protected:
  vtkCPUniformGridBuilder();
  ~vtkCPUniformGridBuilder();

private:
  vtkCPUniformGridBuilder(const vtkCPUniformGridBuilder&) = delete;
  void operator=(const vtkCPUniformGridBuilder&) = delete;

  /**
   * The dimensions of the vtkUniformGrid.
   */
  int Dimensions[3];

  /**
   * The spacing of the vtkUniformGrid.
   */
  double Spacing[3];

  /**
   * The origin of the vtkUniformGrid.
   */
  double Origin[3];

  /**
   * The uniform grid that is created.
   */
  vtkUniformGrid* UniformGrid;

  //@{
  /**
   * Macro to set UniformGrid.
   */
  void SetUniformGrid(vtkUniformGrid* UG);
};
#endif
//@}
