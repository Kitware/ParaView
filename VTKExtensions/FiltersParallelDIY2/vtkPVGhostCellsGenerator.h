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

class vtkDataObject;
class vtkMultiProcessController;
class vtkMultiBlockDataSet;
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
   * Apply the GCG filter to the input.
   * If it is a HTG or a composite dataset containing at least 1 HTG, it will be dispatched to the
   * HTG GCG filter.
   * Assumes output has the same structure as the input
   */
  int ProcessDataObject(vtkDataObject* input, vtkDataObject* output);

  /**
   * Apply the GCG filter to the composite input recursively if it contains at least one HTG.
   * Partitioned DataSets will be processed toghether.
   * Assumes output has the same structure as the input
   */
  int ProcessPartitionedDataSet(vtkCompositeDataSet* input, vtkCompositeDataSet* output);

  /**
   * Apply the GCG filter to the multiblock input if it contains at least one HTG.
   * Multiblock DataSets will be processed individually.
   * Assumes output has the same structure as the input
   */
  int ProcessMultiBlockDataSet(vtkMultiBlockDataSet* input, vtkMultiBlockDataSet* output);
};

#endif
