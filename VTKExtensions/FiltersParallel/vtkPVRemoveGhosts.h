// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVRemoveGhosts
 * @brief Remove ghost information on input vtkPolyData, vtkUnstructuredGrid or vtkHyperTreeGrid.
 *
 * This meta-filter add support of vtkHyperTreeGrid. It forwards ghost information removal
 * to the the appropriate VTK filter.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
