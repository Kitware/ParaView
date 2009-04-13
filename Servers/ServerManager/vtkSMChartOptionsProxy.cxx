/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartOptionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtChartView.h"

vtkStandardNewMacro(vtkSMChartOptionsProxy);
vtkCxxRevisionMacro(vtkSMChartOptionsProxy, "1.2");
vtkCxxSetObjectMacro(vtkSMChartOptionsProxy, ChartView, vtkQtChartView);
//----------------------------------------------------------------------------
vtkSMChartOptionsProxy::vtkSMChartOptionsProxy()
{
  this->ChartView = 0;
}

//----------------------------------------------------------------------------
vtkSMChartOptionsProxy::~vtkSMChartOptionsProxy()
{
  this->SetChartView(0);
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetTitle(const char* title)
{
  if (this->ChartView)
    {
    this->ChartView->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetTitleFont(
  const char* family, int pointSize, bool bold, bool italic)
{
  if (this->ChartView)
    {
    this->ChartView->SetTitleFont(family, pointSize, bold, italic);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetTitleColor(
  double red, double green, double blue)
{
  if (this->ChartView)
    {
    this->ChartView->SetTitleColor(red, green, blue);
    }
}


//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetTitleAlignment(int alignment)
{
  if (this->ChartView)
    {
    this->ChartView->SetTitleAlignment(alignment);
    }
}


//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisTitle(int index, const char* title)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisTitle(index, title);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisTitleFont(
  int index, const char* family, int pointSize, bool bold, bool italic)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisTitleFont(index, family, pointSize, bold, italic);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisTitleColor(
  int index, double red, double green, double blue)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisTitleColor(index, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisTitleAlignment(int index, int alignment)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisTitleAlignment(index, alignment);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetLegendVisibility(bool visible)
{
  if (this->ChartView)
    {
    this->ChartView->SetLegendVisibility(visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetLegendLocation(int location)
{
  if (this->ChartView)
    {
    this->ChartView->SetLegendLocation(location);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetLegendFlow(int flow)
{
  if (this->ChartView)
    {
    this->ChartView->SetLegendFlow(flow);
    }
}


//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisVisibility(int index, bool visible)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisVisibility(index, visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisColor(
  int index, double red, double green, double blue)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisColor(index, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetGridVisibility(int index, bool visible)
{
  if (this->ChartView)
    {
    this->ChartView->SetGridVisibility(index, visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetGridColorType(int index, int gridColorType)
{
  if (this->ChartView)
    {
    this->ChartView->SetGridColorType(index, gridColorType);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetGridColor(
  int index, double red, double green, double blue)
{
  if (this->ChartView)
    {
    this->ChartView->SetGridColor(index, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisLabelVisibility(int index, bool visible)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisLabelVisibility(index, visible);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisLabelFont(
  int index, const char* family, int pointSize, bool bold, bool italic)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisLabelFont(index, family, pointSize, bold, italic);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisLabelColor(
  int index, double red, double green, double blue)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisLabelColor(index, red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisLabelNotation(int index, int notation)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisLabelNotation(index, notation);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisLabelPrecision(int index, int precision)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisLabelPrecision(index, precision);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisScale(int index, int scale)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisScale(index, scale);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisBehavior(int index, int behavior)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisBehavior(index, behavior);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisRange(
  int index, double minimum, double maximum)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisRange(index, minimum, maximum);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisRange(int index, int minimum, int maximum)
{
  if (this->ChartView)
    {
    this->ChartView->SetAxisRange(index, minimum, maximum);
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


