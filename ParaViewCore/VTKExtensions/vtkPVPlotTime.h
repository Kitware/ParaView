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
// .NAME vtkPVPlotTime - takes care of drawing a "time" marker in the plot.
// .SECTION Description
// vtkPVPlotTime is used to add a "current-time" marker to the plot when on of
// the axes in the plots is time. Currently only X-axis as time is supported.

#ifndef __vtkPVPlotTime_h
#define __vtkPVPlotTime_h

#include "vtkPlot.h"

class VTK_EXPORT vtkPVPlotTime : public vtkPlot
{
public:
  static vtkPVPlotTime* New();
  vtkTypeMacro(vtkPVPlotTime, vtkPlot);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  enum
    {
    NONE=0,
    X_AXIS=1,
    Y_AXIS=2
    };

  // Description:
  // Set the Time axis mode.
  vtkSetClampMacro(TimeAxisMode, int, NONE, Y_AXIS);
  vtkGetMacro(TimeAxisMode, int);

  // Description:
  // Set time value.
  vtkSetMacro(Time, double);
  vtkGetMacro(Time, double);

  // Description:
  // Paint event for the axis, called whenever the axis needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
  virtual void GetBounds(double bounds[4])
  { bounds[0] = bounds[2] = 1.0; bounds[1] = bounds[3] = -1.0;}

//BTX
protected:
  vtkPVPlotTime();
  ~vtkPVPlotTime();

  double Time;
  int TimeAxisMode;
private:
  vtkPVPlotTime(const vtkPVPlotTime&); // Not implemented
  void operator=(const vtkPVPlotTime&); // Not implemented
//ETX
};

#endif
