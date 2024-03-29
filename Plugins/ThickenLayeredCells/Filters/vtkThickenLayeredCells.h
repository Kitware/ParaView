// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkThickenLayeredCells
// .SECTION Description
// vtkThickenLayeredCells is a filter that thickens cells processing them in
// layers (highest to lowest) using average thickeness. Both thickness and
// layers information is expected as cell-data in the input dataset.
// Currently this filter only supports wedges.

#ifndef vtkThickenLayeredCells_h
#define vtkThickenLayeredCells_h

#include "vtkThickenLayeredCellsFiltersModule.h" // for export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKTHICKENLAYEREDCELLSFILTERS_EXPORT vtkThickenLayeredCells
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkThickenLayeredCells* New();
  vtkTypeMacro(vtkThickenLayeredCells, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Enable/disable thickening.
  vtkSetMacro(EnableThickening, bool);
  vtkGetMacro(EnableThickening, bool);

protected:
  vtkThickenLayeredCells();
  ~vtkThickenLayeredCells() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  bool EnableThickening;

private:
  vtkThickenLayeredCells(const vtkThickenLayeredCells&) = delete;
  void operator=(const vtkThickenLayeredCells&) = delete;
};

#endif
