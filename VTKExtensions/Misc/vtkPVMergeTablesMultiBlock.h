/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMergeTablesMultiBlock.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVMergeTablesMultiBlock
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
 * The output is a multiblock dataset of vtkTable.
 * @todo
 * We may want to consolidate with vtkPVMergeTable somehow
*/

#ifndef vtkPVMergeTablesMultiBlock_h
#define vtkPVMergeTablesMultiBlock_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsMiscModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSMISC_EXPORT vtkPVMergeTablesMultiBlock
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPVMergeTablesMultiBlock* New();
  vtkTypeMacro(vtkPVMergeTablesMultiBlock, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVMergeTablesMultiBlock();
  ~vtkPVMergeTablesMultiBlock() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVMergeTablesMultiBlock(const vtkPVMergeTablesMultiBlock&) = delete;
  void operator=(const vtkPVMergeTablesMultiBlock&) = delete;
};

#endif
