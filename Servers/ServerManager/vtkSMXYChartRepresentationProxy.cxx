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
#include "vtkSMChartNamedOptionsModelProxy.h"
#include "vtkSMXYChartViewProxy.h"
#include "vtkContextView.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"

vtkStandardNewMacro(vtkSMXYChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMXYChartRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::vtkSMXYChartRepresentationProxy()
{
  //this->VTKRepresentation = vtkQtChartRepresentation::New();
  this->Visibility = 1;

  this->UseIndexForXAxis = true;
  this->XSeriesName = 0;
}

//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::~vtkSMXYChartRepresentationProxy()
{
  this->SetXSeriesName(0);
  //this->VTKRepresentation->Delete();
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
/*
  this->OptionsProxy = vtkSMChartNamedOptionsModelProxy::SafeDownCast(
    this->GetSubProxy("SeriesOptions"));
  this->OptionsProxy->CreateObjects(this->VTKRepresentation);
  this->VTKRepresentation->SetOptionsModel(
    this->OptionsProxy->GetOptionsModel()); */
  return true;
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
  vtkChartXY* chart = chartView->GetChartXY();

  this->ChartViewProxy = chartView;
  if (this->Visibility && chart)
    {
//    this->ChartViewProxy->GetChartView()->AddRepresentation(
//      this->VTKRepresentation);
    }
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

  if (this->Visibility && this->ChartViewProxy)
    {
//    this->ChartViewProxy->GetChartView()->RemoveRepresentation(
//      this->VTKRepresentation);
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
  vtkSMXYChartViewProxy* chartView = vtkSMXYChartViewProxy::SafeDownCast(view);
  if (!chartView)
    {
    return;
    }
  vtkChartXY* chart = chartView->GetChartXY();
  if (!chart)
    {
    return;
    }
  // Add a plot and set up the series
  vtkPlot *plot = chart->AddPlot(vtkChart::LINE);
  plot->SetInput(vtkTable::SafeDownCast(this->GetOutput()), 2, 0);

//  this->VTKRepresentation->SetInputConnection(
//    this->GetOutput()->GetProducerPort());
//  this->VTKRepresentation->Update();
  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
int vtkSMXYChartRepresentationProxy::GetNumberOfSeries()
{
//  return this->VTKRepresentation->GetNumberOfSeries();
}

//----------------------------------------------------------------------------
const char* vtkSMXYChartRepresentationProxy::GetSeriesName(int series)
{
//  return this->VTKRepresentation->GetSeriesName(series);
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetUseIndexForXAxis(bool use_index)
{
  this->UseIndexForXAxis = use_index;
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetXAxisSeriesName(const char* name)
{
  this->SetXSeriesName(name);
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::UpdateXSeriesName()
{
  if (!this->UseIndexForXAxis && this->XSeriesName && this->XSeriesName[0])
    {
//    this->VTKRepresentation->SetKeyColumn(this->XSeriesName);
    }
  else
    {
//    this->VTKRepresentation->SetKeyColumn(0);
    }
}
