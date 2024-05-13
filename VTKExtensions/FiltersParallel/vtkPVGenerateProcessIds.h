// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGenerateProcessIds
 * @brief Generate process IDs of input vtkDataSet or vtkHyperTreeGrid.
 *
 * This meta-filter add support of vtkHyperTreeGrid by forwarding process IDs
 * generation to the appropriate VTK filter.
 *
 * @sa vtkGenerateProcessIds vtkHyperTreeGridGenerateProcessIds
 */

#ifndef vtkPVGenerateProcessIds_h
#define vtkPVGenerateProcessIds_h

#include "vtkPVVTKExtensionsFiltersParallelModule.h" //needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSPARALLEL_EXPORT vtkPVGenerateProcessIds
  : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPVGenerateProcessIds* New();
  vtkTypeMacro(vtkPVGenerateProcessIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether to generate process ids for PointData.
   * Default is true.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   * In that case, point data is never generated.
   */
  vtkSetMacro(GeneratePointData, bool);
  vtkGetMacro(GeneratePointData, bool);
  vtkBooleanMacro(GeneratePointData, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to generate process ids for CellData.
   * Default is false.
   *
   * @note: Unused if input is a vtkHyperTreeGrid instance.
   * In that case, cell data is always generated.
   */
  vtkSetMacro(GenerateCellData, bool);
  vtkGetMacro(GenerateCellData, bool);
  vtkBooleanMacro(GenerateCellData, bool);
  ///@}

protected:
  vtkPVGenerateProcessIds() = default;
  ~vtkPVGenerateProcessIds() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkPVGenerateProcessIds(const vtkPVGenerateProcessIds&) = delete;
  void operator=(const vtkPVGenerateProcessIds&) = delete;

  bool GeneratePointData = true;
  bool GenerateCellData = false;
};

#endif
