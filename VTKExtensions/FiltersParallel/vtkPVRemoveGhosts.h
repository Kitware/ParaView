// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVRemoveGhosts
 * @brief Remove ghost information on input vtkPolyData, vtkUnstructuredGrid or vtkHyperTreeGrid.
 *
 * This meta-filter removes ghost information on input vtkPolyData, vtkUnstructuredGrid
 * or vtkHyperTreeGrid by forwarding the work to the the appropriate VTK filter, depending
 * on the input data type.
 *
 * @sa vtkRemoveGhosts vtkHyperTreeGridRemoveGhostCells
 */

#ifndef vtkPVRemoveGhosts_h
#define vtkPVRemoveGhosts_h

#include "vtkPVVTKExtensionsFiltersParallelModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSPARALLEL_EXPORT vtkPVRemoveGhosts : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPVRemoveGhosts* New();
  vtkTypeMacro(vtkPVRemoveGhosts, vtkPassInputTypeAlgorithm);

protected:
  vtkPVRemoveGhosts() = default;
  ~vtkPVRemoveGhosts() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkPVRemoveGhosts(const vtkPVRemoveGhosts&) = delete;
  void operator=(const vtkPVRemoveGhosts&) = delete;
};

#endif
