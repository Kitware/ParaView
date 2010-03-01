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
#include "vtkAxis.h"

#include "vtkstd/string"

vtkStandardNewMacro(vtkSMXYChartViewProxy);
vtkCxxRevisionMacro(vtkSMXYChartViewProxy, "1.3");
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
void vtkSMXYChartViewProxy::SetTitle(const char* title)
{
  if (this->Chart)
    {
    this->Chart->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetTitleFont(const char* family, int pointSize,
                                         bool bold, bool italic)
{

}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetTitleColor(double red, double green, double blue)
{

}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetTitleAlignment(int alignment)
{

}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetLegendVisibility(int visible)
{
  if (this->Chart)
    {
    this->Chart->SetShowLegend(static_cast<bool>(visible));
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetGridVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    //this->ChartView->SetGridVisibility(index, visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLogScale(int index, bool logScale)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLogScale(logScale);
    this->Chart->Update();
    this->Chart->RecalculateBounds();
    }
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
