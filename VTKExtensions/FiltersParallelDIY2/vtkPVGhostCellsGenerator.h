// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGhostCellsGenerator
 * @brief Ghost Cells Generator that add support for vtkHyperTreeGrid.
 *
 * This is a subclass of vtkGhostCellsGenerator that allows selection of
 * input vtkHyperTreeGrid
 */

#ifndef vtkPVGhostCellsGenerator_h
#define vtkPVGhostCellsGenerator_h

#include "vtkGhostCellsGenerator.h"
#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" //needed for exports

class VTKPVVTKEXTENSIONSFILTERSPARALLELDIY2_EXPORT vtkPVGhostCellsGenerator
  : public vtkGhostCellsGenerator
{
public:
  static vtkPVGhostCellsGenerator* New();
  vtkTypeMacro(vtkPVGhostCellsGenerator, vtkGhostCellsGenerator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVGhostCellsGenerator() = default;
  ~vtkPVGhostCellsGenerator() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int GhostCellsGeneratorUsingSuperclassInstance(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkPVGhostCellsGenerator(const vtkPVGhostCellsGenerator&) = delete;
  void operator=(const vtkPVGhostCellsGenerator&) = delete;
};

#endif
