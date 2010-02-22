/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXYChartViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkChartXY.h"

#include "vtkstd/string"

vtkStandardNewMacro(vtkSMXYChartViewProxy);
vtkCxxRevisionMacro(vtkSMXYChartViewProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMXYChartViewProxy::vtkSMXYChartViewProxy()
{
  this->Chart = NULL;
}

//----------------------------------------------------------------------------
vtkSMXYChartViewProxy::~vtkSMXYChartViewProxy()
{
  if (this->Chart)
    {
    this->Chart->Delete();
    this->Chart = NULL;
    }
}

//----------------------------------------------------------------------------
vtkContextView* vtkSMXYChartViewProxy::NewChartView()
{
  // Construct a new chart view and return the view of it
  this->Chart = vtkChartXY::New();
  this->ChartView->GetScene()->AddItem(this->Chart);

  return this->ChartView;
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetChartType(const char *)
{
  // Does this proxy need to remember what type it is?
}

//----------------------------------------------------------------------------
vtkChartXY* vtkSMXYChartViewProxy::GetChartXY()
{
  return this->Chart;
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::PerformRender()
{
  if (!this->Chart)
    {
    return;
    }

  this->ChartView->Render();
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
