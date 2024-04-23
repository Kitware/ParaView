// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVGenerateGlobalIds
 * @brief Generate global points and cell IDs
 *
 * vtkGenerateGlobalIds generates global point and cell ids of input vtkDataSet, or
 * global cell ids only of input vtkHyperTreeGrid. If the input is a vtkDataSet instance,
 * this filter also generates ghost-point information, flagging duplicate points appropriately.
 * vtkPVGenerateGlobalIds works across all blocks in the input datasets and across all ranks.
 *
 * @sa vtkGenerateGlobalIds vtkHyperTreeGridGenerateGlobalIds
 */

#ifndef vtkPVGenerateGlobalIds_h
#define vtkPVGenerateGlobalIds_h

#include "vtkPVVTKExtensionsFiltersParallelDIY2Module.h" // needed for exports
#include "vtkPassInputTypeAlgorithm.h"

class VTKPVVTKEXTENSIONSFILTERSPARALLELDIY2_EXPORT vtkPVGenerateGlobalIds
  : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPVGenerateGlobalIds* New();
  vtkTypeMacro(vtkPVGenerateGlobalIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the tolerance to use to identify coincident points. 0 means the
   * points should be exactly identical.
   *
   * Default is 0.
   *
   * @note Unused if the input is a vtkHyperTreeGrid instance.
   */
  vtkSetClampMacro(Tolerance, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  vtkPVGenerateGlobalIds() = default;
  ~vtkPVGenerateGlobalIds() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkPVGenerateGlobalIds(const vtkPVGenerateGlobalIds&) = delete;
  void operator=(const vtkPVGenerateGlobalIds&) = delete;

  double Tolerance = 0.0;
};

#endif
