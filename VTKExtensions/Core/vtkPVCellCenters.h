// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVCellCenters
 * @brief Cell centers filter that delegates to typen specific implementations.
 *
 * This is a meta filter of vtkCellCenters that allows selection of
 * input vtkHyperTreeGrid or vtkDataSet
 */

#ifndef vtkPVCellCenters_h
#define vtkPVCellCenters_h

#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include "vtkPolyDataAlgorithm.h"

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVCellCenters : public vtkPolyDataAlgorithm
{

public:
  static vtkPVCellCenters* New();
  vtkTypeMacro(vtkPVCellCenters, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  vtkSetMacro(VertexCells, bool);
  vtkGetMacro(VertexCells, bool);
  vtkBooleanMacro(VertexCells, bool);
  ///@}

protected:
  vtkPVCellCenters() = default;
  ~vtkPVCellCenters() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  bool VertexCells = false;

private:
  vtkPVCellCenters(const vtkPVCellCenters&) = delete;
  void operator=(const vtkPVCellCenters&) = delete;
};

#endif
