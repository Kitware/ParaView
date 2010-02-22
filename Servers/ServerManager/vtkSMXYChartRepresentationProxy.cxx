/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXYChartRepresentationProxy.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMContextNamedOptionsProxy.h"
#include "vtkSMXYChartViewProxy.h"
#include "vtkContextView.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkSMXYChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMXYChartRepresentationProxy, "1.7");
//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::vtkSMXYChartRepresentationProxy()
{
  this->Visibility = 1;
}

//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::~vtkSMXYChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMXYChartRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  // The reduction type for all chart representation is TABLE_MERGE since charts
  // always deliver tables.
  this->SetReductionType(vtkSMClientDeliveryRepresentationProxy::TABLE_MERGE);

  this->OptionsProxy = vtkSMContextNamedOptionsProxy::SafeDownCast(
    this->GetSubProxy("PlotOptions"));
  if (this->OptionsProxy)
    {
    this->OptionsProxy->SetChart(this->GetChart());
    this->OptionsProxy->SetTable(vtkTable::SafeDownCast(this->GetOutput()));
    }

  return true;
}

//----------------------------------------------------------------------------
vtkChartXY* vtkSMXYChartRepresentationProxy::GetChart()
{
  if (this->ChartViewProxy)
    {
    return this->ChartViewProxy->GetChartXY();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
bool vtkSMXYChartRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  vtkSMXYChartViewProxy* chartView = vtkSMXYChartViewProxy::SafeDownCast(view);
  if (!chartView)
    {
    return false;
    }
  this->ChartViewProxy = chartView;

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMXYChartRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMXYChartViewProxy* chartView = vtkSMXYChartViewProxy::SafeDownCast(view);
  if (!chartView || chartView != this->ChartViewProxy)
    {
    return false;
    }

  if (this->Visibility && this->GetChart())
    {
      this->GetChart()->ClearPlots();
    }
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    if (this->ChartViewProxy)
      {
      if (this->Visibility)
        {
//        this->ChartViewProxy->GetChartView()->AddRepresentation(
//          this->VTKRepresentation);
        }
      else
        {
//        this->ChartViewProxy->GetChartView()->RemoveRepresentation(
//          this->VTKRepresentation);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);

  this->OptionsProxy->SetChart(this->GetChart());
  this->OptionsProxy->SetTable(vtkTable::SafeDownCast(this->GetOutput()));

  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
int vtkSMXYChartRepresentationProxy::GetNumberOfSeries()
{
  vtkTable *table = vtkTable::SafeDownCast(this->GetOutput());
  if (table)
    {
    return table->GetNumberOfColumns();
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
const char* vtkSMXYChartRepresentationProxy::GetSeriesName(int col)
{
  vtkTable *table = vtkTable::SafeDownCast(this->GetOutput());
  if (table)
    {
    return table->GetColumnName(col);
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetXAxisSeriesName(const char* name)
{
  this->OptionsProxy->SetXSeriesName(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetUseIndexForXAxis(bool useIndex)
{
  this->OptionsProxy->SetUseIndexForXAxis(useIndex);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetChartType(const char *type)
{
  // Match the string to the vtkChart enum, if no match then stick with default.
  if (strcmp(type, "Line") == 0)
    {
    this->OptionsProxy->SetChartType(vtkChart::LINE);
    }
  else if (strcmp(type, "Bar") == 0)
    {
    this->OptionsProxy->SetChartType(vtkChart::BAR);
    }
}

//----------------------------------------------------------------------------
int vtkSMXYChartRepresentationProxy::GetChartType()
{
  return this->OptionsProxy->GetChartType();
}
