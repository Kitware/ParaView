/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVPlotTime
 * @brief   takes care of drawing a "time" marker in the plot.
 *
 * vtkPVPlotTime is used to add a "current-time" marker to the plot when on of
 * the axes in the plots is time. Currently only X-axis as time is supported.
*/

#ifndef vtkPVPlotTime_h
#define vtkPVPlotTime_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkPlot.h"

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVPlotTime : public vtkPlot
{
public:
  static vtkPVPlotTime* New();
  vtkTypeMacro(vtkPVPlotTime, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    NONE = 0,
    X_AXIS = 1,
    Y_AXIS = 2
  };

  //@{
  /**
   * Set the Time axis mode.
   */
  vtkSetClampMacro(TimeAxisMode, int, NONE, Y_AXIS);
  vtkGetMacro(TimeAxisMode, int);
  //@}

  //@{
  /**
   * Set time value.
   */
  vtkSetMacro(Time, double);
  vtkGetMacro(Time, double);
  //@}

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetBounds(double bounds[4]) override
  {
    bounds[0] = bounds[2] = 1.0;
    bounds[1] = bounds[3] = -1.0;
  }

protected:
  vtkPVPlotTime();
  ~vtkPVPlotTime() override;

  double Time;
  int TimeAxisMode;

private:
  vtkPVPlotTime(const vtkPVPlotTime&) = delete;
  void operator=(const vtkPVPlotTime&) = delete;
};

#endif
