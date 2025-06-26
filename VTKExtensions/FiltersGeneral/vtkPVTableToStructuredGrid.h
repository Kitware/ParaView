// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVTableToStructuredGrid
 * @brief Filter that converts table to structured grid
 *
 * This is a subclass of vtkTableToStructuredGrid that allows specifying
 * the data dimensions instead of whole extent.
 */

#ifndef vtkPVTableToStructuredGrid_h
#define vtkPVTableToStructuredGrid_h

#include "vtkPVVTKExtensionsFiltersGeneralModule.h"
#include "vtkTableToStructuredGrid.h"

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVTableToStructuredGrid
  : public vtkTableToStructuredGrid
{
public:
  vtkTypeMacro(vtkPVTableToStructuredGrid, vtkTableToStructuredGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVTableToStructuredGrid* New();

  /**
   * Dimensions defines the size of the data, and MinimumIndex defines
   * the starting index. Together, they give the DataExtent as:
   *   DataExtent[i] = MinimumIndex[i] + Dimensions[i] - 1
   */
  vtkGetVector3Macro(Dimensions, int);
  void SetDimensions(int xdim, int ydim, int zdim);

  vtkGetVector3Macro(MinimumIndex, int);
  void SetMinimumIndex(int xmin, int ymin, int zmin);

protected:
  vtkPVTableToStructuredGrid();
  ~vtkPVTableToStructuredGrid() override = default;

  int Dimensions[3];
  int MinimumIndex[3];

private:
  vtkPVTableToStructuredGrid(const vtkPVTableToStructuredGrid&) = delete;
  void operator=(const vtkPVTableToStructuredGrid&) = delete;
};

#endif
