// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMergeTablesComposite
 * @brief   used to merge rows in tables from composite datasets.
 *
 * This filter operates on composite datasets and simply combines tables by merging
 * columns. The output will have the intersection of all columns from all tables
 * that have at least one row.
 *
 * The merging behavior is controlled by the MergeStrategy property:
 * - ALL: All tables from all leaf nodes across all inputs are merged into a
 *   composite dataset with a single vtkTable
 * - LEAVES: Tables are merged per corresponding leaf node, preserving the
 *   composite structure.
 *
 * This assumes that when using LEAVES strategy, all inputs have the same
 * composite structure.
 * All inputs must be a vtkMultiBlockDataSet/vtkPartitionedDataSetCollection of vtkTables.
 * The output is a vtkMultiBlockDataSet/vtkPartitionedDataSetCollection of vtkTables.
 */

#ifndef vtkPVMergeTablesComposite_h
#define vtkPVMergeTablesComposite_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

#include <vector> // for std::vector

class vtkDataObjectTree;

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVMergeTablesComposite : public vtkDataObjectAlgorithm
{
public:
  static vtkPVMergeTablesComposite* New();
  vtkTypeMacro(vtkPVMergeTablesComposite, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum MergeStrategies
  {
    ALL = 0,
    LEAVES = 1,
  };

  ///@{
  /**
   * MergeStrategy indicates the merging strategy used for merging the tables.
   * All means that all block tables are merged, while leaves means that tables
   * are merged only per leaf node. Default is leaves.
   */
  vtkGetMacro(MergeStrategy, int);
  vtkSetClampMacro(MergeStrategy, int, ALL, LEAVES);
  ///@}

protected:
  vtkPVMergeTablesComposite();
  ~vtkPVMergeTablesComposite() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVMergeTablesComposite(const vtkPVMergeTablesComposite&) = delete;
  void operator=(const vtkPVMergeTablesComposite&) = delete;

  void MergeAllTables(const std::vector<vtkDataObjectTree*>& inputs, vtkDataObjectTree* output);
  void MergeLeavesTables(const std::vector<vtkDataObjectTree*>& inputs, vtkDataObjectTree* output);

  int MergeStrategy = LEAVES; // Default to merging all blocks.
};

#endif
