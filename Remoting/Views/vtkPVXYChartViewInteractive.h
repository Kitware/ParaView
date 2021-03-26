/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYChartViewInteractive.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkPVXYChartViewInteractive_h
#define vtkPVXYChartViewInteractive_h

#include "vtkPVXYChartView.h"

/**
 * @class vtkPVXYChartViewInteractive
 * @brief The vtkPVXYChartViewInteractive repeats the functionality of vtkPVXYChartView and allows
 * you to add interactive 2D widgets.
 */

class VTKREMOTINGVIEWS_EXPORT vtkPVXYChartViewInteractive : public vtkPVXYChartView
{
public:
  static vtkPVXYChartViewInteractive* New();
  vtkTypeMacro(vtkPVXYChartViewInteractive, vtkPVXYChartView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPVXYChartViewInteractive() = default;
  ~vtkPVXYChartViewInteractive() override = default;

private:
  vtkPVXYChartViewInteractive(const vtkPVXYChartViewInteractive&) = delete;
  void operator=(const vtkPVXYChartViewInteractive&) = delete;
};

#endif
