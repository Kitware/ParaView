// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGhostCellsGenerator
 * @brief Ghost Cells Generator that adds support for vtkHyperTreeGrid.
 *
 * Meta class that switches between ghost cells generator filters to
 * select the right one depending on the input type.
 */

#ifndef vtkPVGhostCellsGenerator_h
#define vtkPVGhostCellsGenerator_h

#include "vtkPVDataSetAlgorithmSelectorFilter.h"
#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" //needed for exports

#include <memory> // For unique_ptr

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSFILTERSPARALLELDIY2_EXPORT vtkPVGhostCellsGenerator
  : public vtkPVDataSetAlgorithmSelectorFilter
{
public:
  static vtkPVGhostCellsGenerator* New();
  vtkTypeMacro(vtkPVGhostCellsGenerator, vtkPVDataSetAlgorithmSelectorFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   * Does not apply to Hyper Tree Grid.
   */
  void SetController(vtkMultiProcessController* controller);

  /**
   * Specify if the filter must generate the ghost cells only if required by
   * the pipeline.
   * If false, ghost cells are computed even if they are not required.
   * Does not apply to Hyper Tree Grid.
   * Default is TRUE.
   */
  void SetBuildIfRequired(bool enable);

  /**
   * When BuildIfRequired is `false`, this can be used to set the number
   * of ghost layers to generate. Note, if the downstream pipeline requests more
   * ghost levels than the number specified here, then the filter will generate
   * those extra ghost levels as needed. Accepted values are in the interval
   * [1, VTK_INT_MAX].
   * Does not apply to Hyper Tree Grid.
   * Default is 1.
   */
  void SetNumberOfGhostLayers(int nbGhostLayers);

  /**
   * Specify if the filter should try to synchronize ghost data
   * without recomputing ghost cells localization.
   * If On, it assumes the number of ghost layer should not change.
   * If On, but conditions are not met (ghosts, gids and pids), it
   * will fallback on the default behavior: generating ghosts.
   * Does not apply to Hyper Tree Grid.
   * Pass the call to the internal vtkGhostCellGenerator.
   */
  void SetSynchronizeOnly(bool sync);

  /**
   * Specify if the filter should generate GlobalsIds.
   * Default is false.
   */
  void SetGenerateGlobalIds(bool set);

  /**
   * Specify if the filter should generate ProcessIds.
   * Default is false.
   */
  void SetGenerateProcessIds(bool set);

protected:
  vtkPVGhostCellsGenerator();
  ~vtkPVGhostCellsGenerator() override;

private:
  vtkPVGhostCellsGenerator(const vtkPVGhostCellsGenerator&) = delete;
  void operator=(const vtkPVGhostCellsGenerator&) = delete;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
