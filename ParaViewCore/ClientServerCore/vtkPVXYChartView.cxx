/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXYChartView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXYChartView.h"

#include "vtkAxis.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkChartXY.h"
#include "vtkChartLegend.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPVPlotTime.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkXYChartRepresentation.h"

#include "string"
#include "vtksys/ios/sstream"

#include "vtkCommand.h"

// Command implementation
class vtkPVXYChartView::CommandImpl : public vtkCommand
{
public:
  static CommandImpl* New(vtkPVXYChartView *proxy)
  {
    return new CommandImpl(proxy);
  }

  CommandImpl(vtkPVXYChartView* proxy)
    : Target(proxy), Initialized(false)
  { }

  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target->SelectionChanged();
  }
  vtkPVXYChartView* Target;
  bool Initialized;
};

vtkStandardNewMacro(vtkPVXYChartView);

//----------------------------------------------------------------------------
vtkPVXYChartView::vtkPVXYChartView()
{
  this->Chart = NULL;
  this->InternalTitle = NULL;
  this->Command = CommandImpl::New(this);
  this->PlotTime = vtkPVPlotTime::New();

  // Use the buffer id - performance issues are fixed.
  this->ContextView->GetScene()->SetUseBufferId(true);
  this->ContextView->GetScene()->SetScaleTiles(false);
}

//----------------------------------------------------------------------------
vtkPVXYChartView::~vtkPVXYChartView()
{
  if (this->Chart)
    {
    this->Chart->Delete();
    this->Chart = NULL;
    }
  this->PlotTime->Delete();
  this->PlotTime = NULL;

  this->SetInternalTitle(NULL);
  this->Command->Delete();
}

//----------------------------------------------------------------------------
vtkAbstractContextItem* vtkPVXYChartView::GetContextItem()
{
  return this->GetChart();
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetChartType(const char *type)
{
  if (this->Chart)
    {
    this->Chart->Delete();
    this->Chart = NULL;
    }

  // Construct the correct type of chart
  if (strcmp(type, "Line") == 0 || strcmp(type, "Bar") == 0)
    {
    this->Chart = vtkChartXY::New();
    }
  else if (strcmp(type, "ParallelCoordinates") == 0)
    {
    this->Chart = vtkChartParallelCoordinates::New();
    }

  if (this->Chart)
    {
    // Default to empty axis titles
    this->SetAxisTitle(0, "");
    this->SetAxisTitle(1, "");
    this->Chart->AddPlot(this->PlotTime);

    this->Chart->AddObserver(vtkCommand::SelectionChangedEvent, this->Command);
    this->ContextView->GetScene()->AddItem(this->Chart);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitle(const char* title)
{
  if (this->Chart)
    {
    std::string tmp(title);
    if (tmp.find("${TIME}") != std::string::npos)
      {
      this->SetInternalTitle(title);
      }
    else
      {
      this->Chart->SetTitle(title);
      this->SetInternalTitle(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitleFont(const char* family, int pointSize,
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
void vtkPVXYChartView::SetTitleColor(double red, double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetColor(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTitleAlignment(int alignment)
{
  if (this->Chart)
    {
    this->Chart->GetTitleProperties()->SetJustification(alignment);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetLegendVisibility(int visible)
{
  if (this->Chart)
    {
    this->Chart->SetShowLegend(visible? true : false);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetLegendLocation(int location)
{
  if (this->Chart)
    {
    vtkChartLegend *legend = this->Chart->GetLegend();
    legend->SetInline(location < 4);
    switch(location)
      {
      case 0: // TOP-LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 1: // TOP-RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 2: // BOTTOM-RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      case 3: // BOTTOM-LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      case 4: // LEFT
        legend->SetHorizontalAlignment(vtkChartLegend::LEFT);
        legend->SetVerticalAlignment(vtkChartLegend::CENTER);
        break;
      case 5: // TOP
        legend->SetHorizontalAlignment(vtkChartLegend::CENTER);
        legend->SetVerticalAlignment(vtkChartLegend::TOP);
        break;
      case 6: // RIGHT
        legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
        legend->SetVerticalAlignment(vtkChartLegend::CENTER);
        break;
      case 7: // BOTTOM
        legend->SetHorizontalAlignment(vtkChartLegend::CENTER);
        legend->SetVerticalAlignment(vtkChartLegend::BOTTOM);
        break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetGridVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetGridVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetGridColor(int index, double red, double green,
                                         double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetGridPen()->SetColorF(red, green, blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelVisibility(int index, bool visible)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLabelsVisible(visible);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelFont(int index, const char* family,
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
void vtkPVXYChartView::SetAxisLabelColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetLabelProperties()->SetColor(red, green,
                                                                blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelNotation(int index, int notation)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetNotation(notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelPrecision(int index, int precision)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetPrecision(precision);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisRange(int index, double min, double max)
{
  if (this->Chart)
    {
    vtkAxis* axis = this->Chart->GetAxis(index);
    axis->SetBehavior(vtkAxis::FIXED);
    axis->SetMinimum(min);
    axis->SetMaximum(max);
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::UnsetAxisRange(int index)
{
  if (this->Chart)
    {
    vtkAxis* axis = this->Chart->GetAxis(index);
    axis->SetBehavior(vtkAxis::AUTO);

    // we set some random min and max so we can notice them when they get used.
    axis->SetMinimum(0.0);
    axis->SetMaximum(6.66);
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLogScale(int index, bool logScale)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->SetLogScale(logScale);
    this->Chart->Update();
    this->Chart->RecalculateBounds();
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisTitle(int index, const char* title)
{
  if (this->Chart && this->Chart->GetAxis(index))
    {
    this->Chart->GetAxis(index)->SetTitle(title);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisTitleFont(int index, const char* family,
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
void vtkPVXYChartView::SetAxisTitleColor(int index, double red,
                                              double green, double blue)
{
  if (this->Chart)
    {
    this->Chart->GetAxis(index)->GetTitleProperties()->SetColor(red, green,
                                                                blue);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsNumber(int axis, int n)
{
  if (this->Chart && this->Chart->GetAxis(axis))
    {
    this->Chart->GetAxis(axis)->GetTickPositions()->SetNumberOfTuples(n);
    this->Chart->GetAxis(axis)->GetTickLabels()->SetNumberOfTuples(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabels(int axisIndex, int i, double value)
{
  if (this->Chart && this->Chart->GetAxis(axisIndex))
    {
    vtkAxis *axis = this->Chart->GetAxis(axisIndex);
    axis->GetTickPositions()->SetTuple1(i, value);
    if (i == 0)
      {
      axis->SetMinimum(value);
      }
    else if (i == axis->GetTickPositions()->GetNumberOfTuples() - 1)
      {
      axis->SetMaximum(value);
      this->Chart->RecalculateBounds();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsLeftNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::LEFT, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsLeft(int i, double value)
{
  this->SetAxisLabels(vtkAxis::LEFT, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsBottomNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::BOTTOM, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsBottom(int i, double value)
{
  this->SetAxisLabels(vtkAxis::BOTTOM, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsRightNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::RIGHT, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsRight(int i, double value)
{
  this->SetAxisLabels(vtkAxis::RIGHT, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsTopNumber(int n)
{
  this->SetAxisLabelsNumber(vtkAxis::TOP, n);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetAxisLabelsTop(int i, double value)
{
  this->SetAxisLabels(vtkAxis::TOP, i, value);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTooltipNotation(int notation)
{
  for(int i = 0; i < this->Chart->GetNumberOfPlots(); i++)
    {
    this->Chart->GetPlot(i)->SetTooltipNotation(notation);
    }
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SetTooltipPrecision(int precision)
{
  for(int i = 0; i < this->Chart->GetNumberOfPlots(); i++)
    {
    this->Chart->GetPlot(i)->SetTooltipPrecision(precision);
    }
}

//----------------------------------------------------------------------------
vtkChart* vtkPVXYChartView::GetChart()
{
  return this->Chart;
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::Render(bool interactive)
{
  if (!this->Chart)
    {
    return;
    }
  if (this->InternalTitle)
    {
    vtksys_ios::ostringstream new_title;
    std::string title(this->InternalTitle);
    size_t pos = title.find("${TIME}");
    if (pos != std::string::npos)
      {
      // The string was found - replace it and set the chart title.
      new_title << title.substr(0, pos)
                << this->GetViewTime()
                << title.substr(pos + strlen("${TIME}"));
      this->Chart->SetTitle(new_title.str().c_str());
      }
    }

  this->PlotTime->SetTime(this->GetViewTime());
  this->PlotTime->SetTimeAxisMode(vtkPVPlotTime::NONE);

  // Decide if time is being shown on any of the axis.
  // Iterate over all visible representations and check is they have the array
  // named "Time" on either of the axes.
  int num_reprs = this->GetNumberOfRepresentations();
  for (int cc=0; cc < num_reprs; cc++)
    {
    vtkXYChartRepresentation * repr = vtkXYChartRepresentation::SafeDownCast(
      this->GetRepresentation(cc));
    if (repr && repr->GetVisibility())
      {
      if (repr->GetXAxisSeriesName() &&
        strcmp(repr->GetXAxisSeriesName(), "Time") == 0)
        {
        this->PlotTime->SetTimeAxisMode(vtkPVPlotTime::X_AXIS);
        break;
        }
      }
    }
  // For now we only handle X-axis time. If needed we can add support for Y-axis.

  this->Superclass::Render(interactive);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::SelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkPVXYChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
