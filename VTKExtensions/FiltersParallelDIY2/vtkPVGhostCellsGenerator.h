// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGhostCellsGenerator
 * @brief Ghost Cells Generator that add support for vtkHyperTreeGrid.
 *
 * This is a subclass of vtkGhostCellsGenerator that can process HyperTreeGrids and their composite
 * derivates. In the case of a composite dataset containing HTG, the output will always be a
 * vtkPartitionedDataSetCollection. Ghost Cells are computed separately for each individual
 * partition/block of the composite structure.
 */

#ifndef vtkPVGhostCellsGenerator_h
#define vtkPVGhostCellsGenerator_h

#include "vtkGhostCellsGenerator.h"
#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" // needed for exports
#include "vtkParaViewDeprecation.h"                      // for PARAVIEW_DEPRECATED_IN_6_0_0

class vtkDataObject;
class vtkMultiProcessController;
class vtkCompositeDataSet;

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

  PARAVIEW_DEPRECATED_IN_6_0_0(
    "Use int GhostCellsGeneratorUsingSuperclassInstance(vtkDataObject*, vtkDataObject*)")
  int GhostCellsGeneratorUsingSuperclassInstance(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Execute classic GCG on the input dataset
   */
  int GhostCellsGeneratorUsingSuperclassInstance(vtkDataObject* inputDO, vtkDataObject* outputDO);

private:
  vtkPVGhostCellsGenerator(const vtkPVGhostCellsGenerator&) = delete;
  void operator=(const vtkPVGhostCellsGenerator&) = delete;

  /**
   * Execute HTG GCG on the input data object.
   */
  int GhostCellsGeneratorUsingHyperTreeGrid(vtkDataObject* inputDO, vtkDataObject* outputDO);

  /**
   * Return true if the object is a HyperTreeGrid or a data object tree containing HyperTreeGrid
   */
  static bool HasHTG(vtkMultiProcessController* controller, vtkDataObject* object);

  /**
   * Apply the GCG filter to the composite input recursively.
   * Partitioned DataSets will be processes toghether.
   * Assumes output has the same structure as the input
   */
  int ProcessComposite(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
};

#endif
