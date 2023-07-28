// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPVBagPlotMatrixRepresentation
 * @brief   vtkPVPlotMatrixRepresentation subclass for
 * BagPlot specific plot matrix representation.
 *
 * vtkPVBagPlotMatrixRepresentation uses vtkPVPlotMatrixRepresentation
 * to draw the plot matrix and extract the explained variance from the data.
 */

#ifndef vtkPVBagPlotMatrixRepresentation_h
#define vtkPVBagPlotMatrixRepresentation_h

#include "vtkBagPlotViewsAndFiltersBagPlotModule.h" // for export macro
#include "vtkPVPlotMatrixRepresentation.h"

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkPVBagPlotMatrixRepresentation
  : public vtkPVPlotMatrixRepresentation
{
public:
  static vtkPVBagPlotMatrixRepresentation* New();
  vtkTypeMacro(vtkPVBagPlotMatrixRepresentation, vtkPVPlotMatrixRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Recover the extracted explained variance from the data
   */
  vtkGetMacro(ExtractedExplainedVariance, double);

protected:
  vtkPVBagPlotMatrixRepresentation() = default;
  ~vtkPVBagPlotMatrixRepresentation() override = default;

  /**
   * Overridden to transfer explained variance from server to client when necessary
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPVBagPlotMatrixRepresentation(const vtkPVBagPlotMatrixRepresentation&) = delete;
  void operator=(const vtkPVBagPlotMatrixRepresentation&) = delete;

  double ExtractedExplainedVariance = -1;
};

#endif
