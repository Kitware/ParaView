// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVExtractGhostCells
 * @brief Meta ghost cell extraction filter to add support for vtkHyperTreeGrid
 *
 * Implementation is provided by either `vtkHyperTreeGridExtractGhostCells` or
 * `vtkExtractGhostCells` depending on the input data type.
 */

#ifndef vtkPVExtractGhostCells_h
#define vtkPVExtractGhostCells_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkInformation;
class vtkInformationVector;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVExtractGhostCells : public vtkDataObjectAlgorithm
{
public:
  static vtkPVExtractGhostCells* New();
  vtkTypeMacro(vtkPVExtractGhostCells, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / Get the name of the ghost cell array in the output.
   */
  vtkSetMacro(OutputGhostArrayName, std::string);
  vtkGetMacro(OutputGhostArrayName, std::string);
  ///@}

protected:
  vtkPVExtractGhostCells() = default;
  ~vtkPVExtractGhostCells() override = default;

  /**
   * Support both `vtkDataSet` and `vtkHyperTreeGrid` as input types
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Dispatch to the right implementation, copying parameters.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Create output data object, according to the input data type
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  std::string OutputGhostArrayName = "GhostType";

  vtkPVExtractGhostCells(const vtkPVExtractGhostCells&) = delete;
  void operator=(const vtkPVExtractGhostCells&) = delete;
};

#endif
