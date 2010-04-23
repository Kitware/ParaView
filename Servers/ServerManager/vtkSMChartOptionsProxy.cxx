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
#include "vtkQtChartArea.h"
#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartView.h"
#include "vtkSMChartViewProxy.h"

vtkStandardNewMacro(vtkSMChartOptionsProxy);
vtkCxxSetObjectMacro(vtkSMChartOptionsProxy, ChartView, vtkQtChartView);
//----------------------------------------------------------------------------
vtkSMChartOptionsProxy::vtkSMChartOptionsProxy()
{
  this->ChartView = 0;
  this->AxisRangesDirty = false;

  for (int cc=0; cc<4;cc++)
    {
    this->AxisBehavior[cc] = 0;
    this->AxisRanges[cc][0] = 0;
    this->AxisRanges[cc][1] = 0;
    }
  this->TitleInternal =0;
  this->SetTitleInternal("");
}

//----------------------------------------------------------------------------
vtkSMChartOptionsProxy::~vtkSMChartOptionsProxy()
{
  this->SetChartView(0);
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::PrepareForRender(vtkSMChartViewProxy* viewProxy)
{
  QString time = QString::number(viewProxy->GetViewUpdateTime());
  QRegExp regExp("\\$\\{TIME\\}", Qt::CaseInsensitive);
  QString title = this->TitleInternal;
  title = title.replace(regExp, time);
  this->ChartView->SetTitle(title.toAscii().data());
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetTitle(const char* title)
{
  this->SetTitleInternal(title);
  // Actual title set in PrepareForRender() to take TIME into consideration if
  // requested.
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
  if (index >=0 && index < 4)
    {
    this->AxisBehavior[index] = behavior;
    this->AxisRangesDirty = true;
    this->Modified();
    this->UpdateAxisRanges();
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::SetAxisRange(
  int index, double minimum, double maximum)
{
  if (index >= 0 && index < 4)
    {
    this->AxisRanges[index][0] = minimum;
    this->AxisRanges[index][1] = maximum;
    this->AxisRangesDirty = true;
    this->Modified();
    this->UpdateAxisRanges();
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::UpdateAxisRanges()
{
  // I am not directly using vtkQtChartView::SetAxisRange or SetAxisBehavior
  // since those methods don't work as expected since the axis ranges etc. are
  // not cached and overridden when behaviour changes, plus the updateLayout()
  // is called way too many times.
  if (this->AxisRangesDirty && this->ChartView)
    {
    vtkQtChartArea* area = this->ChartView->GetChartArea();
    vtkQtChartAxisLayer* axisLayer = area->getAxisLayer();
    bool relayout_needed = false;
    for (int cc=0; cc < 4; cc++)
      {
      if (axisLayer->getAxisBehavior(
          static_cast<vtkQtChartAxis::AxisLocation>(cc)) !=
        this->AxisBehavior[cc])
        {
        relayout_needed = true;
        axisLayer->setAxisBehavior(
          static_cast<vtkQtChartAxis::AxisLocation>(cc), 
          static_cast<vtkQtChartAxisLayer::AxisBehavior>(this->AxisBehavior[cc]));
        }
      vtkQtChartAxis* axis = this->ChartView->GetAxis(cc);
      if (axis && this->AxisBehavior[cc] == vtkQtChartAxisLayer::BestFit)
        {
        QVariant min, max;
        axis->getBestFitRange(min, max);
        if (min.toDouble() != this->AxisRanges[cc][0] || max.toDouble() !=
          this->AxisRanges[cc][1])
          {
          relayout_needed = true;
          axis->setBestFitRange(this->AxisRanges[cc][0], this->AxisRanges[cc][1]);
          }
        axis->setBestFitGenerated(true);
        }
      else
        {
        axis->setBestFitGenerated(false);
        }
      }
    if (relayout_needed)
      {
      area->updateLayout();
      }
    this->AxisRangesDirty = false;
    }
}

//----------------------------------------------------------------------------
void vtkSMChartOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


