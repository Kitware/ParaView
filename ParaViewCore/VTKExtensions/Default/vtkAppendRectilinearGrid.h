/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendRectilinearGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAppendRectilinearGrid
 * @brief   appends rectliner grids together.
 *
 * vtkAppendRectilinearGrid appends rectilinear grids to produce a
 * single combined rectilinear grid. Inputs are appends based on
 * their extents.
*/

#ifndef vtkAppendRectilinearGrid_h
#define vtkAppendRectilinearGrid_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkRectilinearGridAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkAppendRectilinearGrid : public vtkRectilinearGridAlgorithm
{
public:
  static vtkAppendRectilinearGrid* New();
  vtkTypeMacro(vtkAppendRectilinearGrid, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkAppendRectilinearGrid();
  ~vtkAppendRectilinearGrid() override;

  // Propagate UPDATE_EXTENT up to the inputs.
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Tell the output information about the data this filter will produce.
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Perform actual execution.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void CopyArray(
    vtkAbstractArray* outArray, const int* outExt, vtkAbstractArray* inArray, const int* inExt);

private:
  vtkAppendRectilinearGrid(const vtkAppendRectilinearGrid&) = delete;
  void operator=(const vtkAppendRectilinearGrid&) = delete;
};

#endif
