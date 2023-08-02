// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPVBagPlotMatrixView_h
#define vtkPVBagPlotMatrixView_h

#include "vtkBagPlotViewsAndFiltersBagPlotModule.h" // for export macro
#include "vtkPVPlotMatrixView.h"

class VTKBAGPLOTVIEWSANDFILTERSBAGPLOT_EXPORT vtkPVBagPlotMatrixView : public vtkPVPlotMatrixView
{
public:
  static vtkPVBagPlotMatrixView* New();
  vtkTypeMacro(vtkPVBagPlotMatrixView, vtkPVPlotMatrixView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVBagPlotMatrixView() = default;
  ~vtkPVBagPlotMatrixView() override = default;

  /**
   * Rendering implementation.
   */
  void Render(bool interactive) override;

private:
  vtkPVBagPlotMatrixView(const vtkPVBagPlotMatrixView&) = delete;
  void operator=(const vtkPVBagPlotMatrixView&) = delete;
};

#endif
