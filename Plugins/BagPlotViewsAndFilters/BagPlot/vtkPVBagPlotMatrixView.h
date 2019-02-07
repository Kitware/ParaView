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

#include <vtkBagPlotViewsAndFiltersBagPlotModule.h>
#include <vtkPVPlotMatrixView.h>

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
   *
   * Method to get the Formatted title after replacing some key strings
   * After calling the parent class,
   * this replace the key ${VARIANCE} by its actual value
   */
  std::string GetFormattedTitle() override;

private:
  vtkPVBagPlotMatrixView(const vtkPVBagPlotMatrixView&) = delete;
  void operator=(const vtkPVBagPlotMatrixView&) = delete;
};

#endif
