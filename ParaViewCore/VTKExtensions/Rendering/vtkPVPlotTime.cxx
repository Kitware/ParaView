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
#include "vtkPVPlotTime.h"

#include "vtkAxis.h"
#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

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
vtkPVPlotTime::~vtkPVPlotTime()
{
}

//-----------------------------------------------------------------------------
bool vtkPVPlotTime::Paint(vtkContext2D* painter)
{
  if (this->TimeAxisMode == NONE)
  {
    return true;
  }

  painter->ApplyPen(this->GetPen());
  if (this->TimeAxisMode == X_AXIS)
  {
    if (vtkAxis* axis = this->GetYAxis())
    {
      double range[2];
      axis->GetRange(range);
      painter->DrawLine(this->Time, range[0], this->Time, range[1]);
    }
  }
  else
  {
    if (vtkAxis* axis = this->GetXAxis())
    {
      double range[2];
      axis->GetRange(range);
      painter->DrawLine(range[0], this->Time, range[1], this->Time);
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
