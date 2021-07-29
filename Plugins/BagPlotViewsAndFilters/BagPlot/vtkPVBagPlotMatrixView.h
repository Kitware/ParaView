/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBagPlotMatrixView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
