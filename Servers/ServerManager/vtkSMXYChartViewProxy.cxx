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
#include "vtkPen.h"
#include "vtkTextProperty.h"

#include "vtkstd/string"

vtkStandardNewMacro(vtkSMXYChartViewProxy);
vtkCxxRevisionMacro(vtkSMXYChartViewProxy, "1.6");
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
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetFontFamilyAsString(family);
    this->Chart->GetTitleProperties()->SetFontSize(pointSize);
    this->Chart->GetTitleProperties()->SetBold(static_cast<int>(bold));
    this->Chart->GetTitleProperties()->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetTitleColor(double red, double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetColor(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetTitleAlignment(int alignment)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetJustification(alignment);
    }
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
    this->Chart->GetAxis(index)->SetGridVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetGridColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetGridPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLabelVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLabelsVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLabelFont(int index, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  if (this->Chart)
    {
    vtkTextProperty *prop = this->Chart->GetAxis(index)->GetLabelProperties();
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(static_cast<int>(bold));
    prop->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLabelColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetLabelProperties()->SetColor(red, green,
                                                                blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLabelNotation(int index, int notation)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetNotation(notation);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisLabelPrecision(int index, int precision)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetPrecision(precision);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisBehavior(int index, int behavior)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetBehavior(behavior);
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisRange(int index, double min, double max)
{
  if (this->Chart && this->Chart->GetAxis(index)->GetBehavior() > 0)
    {
    this->Chart->GetAxis(index)->SetMinimum(min);
    this->Chart->GetAxis(index)->SetMaximum(max);
    this->Chart->RecalculateBounds();
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
void vtkSMXYChartViewProxy::SetAxisTitle(int index, const char* title)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisTitleFont(int index, const char* family,
                                             int pointSize, bool bold,
                                             bool italic)
{
  if (this->Chart)
    {
    vtkTextProperty *prop = this->Chart->GetAxis(index)->GetTitleProperties();
    prop->SetFontFamilyAsString(family);
    prop->SetFontSize(pointSize);
    prop->SetBold(static_cast<int>(bold));
    prop->SetItalic(static_cast<int>(italic));
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartViewProxy::SetAxisTitleColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetTitleProperties()->SetColor(red, green,
                                                                blue);
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
