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

#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

vtkStandardNewMacro(vtkPVPlotTime);
//----------------------------------------------------------------------------
vtkPVPlotTime::vtkPVPlotTime()
{
  // paraview green.
  this->GetPen()->SetColor(143,216,109);
  this->TimeAxisMode = NONE;
  this->Time = 0.0;
}

//----------------------------------------------------------------------------
vtkPVPlotTime::~vtkPVPlotTime()
{
}

//-----------------------------------------------------------------------------
bool vtkPVPlotTime::Paint(vtkContext2D *painter)
{
  if (this->TimeAxisMode == NONE)
    {
    return true;
    }

  painter->ApplyPen(this->GetPen());
  if (this->TimeAxisMode == X_AXIS)
    {
    // using float max and min for some reason ends up with nothing showing for
    // small scaled plots.
    painter->DrawLine(this->Time, -1.0e+30f, this->Time, 1.0e+30f);
    }
  else
    {
    painter->DrawLine(-1.0e+30f, this->Time, 1.0e+30f, this->Time);
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
