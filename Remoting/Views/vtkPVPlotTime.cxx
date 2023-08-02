// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVPlotTime.h"

#include "vtkAxis.h"
#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

#include <cmath> // for log10

vtkStandardNewMacro(vtkPVPlotTime);
//----------------------------------------------------------------------------
vtkPVPlotTime::vtkPVPlotTime()
{
  // paraview green.
  this->GetPen()->SetColor(143, 216, 109);
  this->TimeAxisMode = NONE;
  this->Time = 0.0;
}

//----------------------------------------------------------------------------
vtkPVPlotTime::~vtkPVPlotTime() = default;

//-----------------------------------------------------------------------------
bool vtkPVPlotTime::Paint(vtkContext2D* painter)
{
  if (this->TimeAxisMode == NONE)
  {
    return true;
  }

  painter->ApplyPen(this->GetPen());
  auto xaxis = this->GetXAxis();
  auto yaxis = this->GetYAxis();
  if (xaxis && yaxis)
  {
    if (this->TimeAxisMode == X_AXIS)
    {
      double range[2];
      yaxis->GetRange(range);

      const bool use_log = xaxis && xaxis->GetLogScaleActive();
      const double x = use_log ? log10(fabs(this->Time)) : this->Time;
      painter->DrawLine(x, range[0], x, range[1]);
    }
    else
    {
      double range[2];
      xaxis->GetRange(range);

      const bool use_log = yaxis && yaxis->GetLogScaleActive();
      const double y = use_log ? log10(fabs(this->Time)) : this->Time;
      painter->DrawLine(range[0], y, range[1], y);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPVPlotTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeAxisMode: " << this->TimeAxisMode << endl;
  os << indent << "Time: " << this->Time << endl;
}
