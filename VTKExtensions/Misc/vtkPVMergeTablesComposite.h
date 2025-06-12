// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVMergeTablesComposite
 * @brief   used to merge rows in tables.
 *
 * Simplified version of vtkMergeTables which simply combines tables merging
 * columns. This assumes that each of the inputs either has exactly identical
 * columns or no columns at all.
 * This filter can handle composite datasets as well. The output is produced by
 * merging corresponding leaf nodes. This assumes that all inputs have the same
 * composite structure.
 * All inputs must either be vtkTable or vtkCompositeDataSet mixing is not
 * allowed.
 * The output is a vtkMultiBlockDataSet/vtkPartitionedDataSetCollection of vtkTables.
 */

#ifndef vtkPVMergeTablesComposite_h
#define vtkPVMergeTablesComposite_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVMergeTablesComposite : public vtkDataObjectAlgorithm
{
public:
  static vtkPVMergeTablesComposite* New();
  vtkTypeMacro(vtkPVMergeTablesComposite, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
};

#endif
