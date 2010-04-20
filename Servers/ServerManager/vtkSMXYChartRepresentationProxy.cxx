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
#include "vtkSMSourceProxy.h"
#include "vtkSMContextNamedOptionsProxy.h"
#include "vtkSMXYChartViewProxy.h"
#include "vtkContextView.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkSMXYChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMXYChartRepresentationProxy, "1.14");
//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::vtkSMXYChartRepresentationProxy()
{
  this->Visibility = 1;
  this->AnnLink = vtkAnnotationLink::New();
}

//----------------------------------------------------------------------------
vtkSMXYChartRepresentationProxy::~vtkSMXYChartRepresentationProxy()
{
  this->AnnLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkSMXYChartRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->SelectionRepresentation =
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(
      this->GetSubProxy("SelectionRepresentation"));
  if (!this->SelectionRepresentation)
    {
    vtkErrorMacro("SelectionRepresentation must be defined in the xml configuration.");
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::CreatePipeline(vtkSMSourceProxy* input,
                                                     int outputport)
{
  this->Superclass::CreatePipeline(input, outputport);

  // Connect the selection output from the input to the SelectionRepresentation.

  vtkSMSourceProxy* realInput = this->GetInputProxy();

  // Ensure that the source proxy has created extract selection filters.
  realInput->CreateSelectionProxies();

  vtkSMSourceProxy* esProxy = realInput->GetSelectionOutput(outputport);
  if (!esProxy)
    {
    vtkErrorMacro("Input proxy does not support selection extraction.");
    return;
    }
  // esProxy port 1 is the input vtkSelection. That's the one we are
  // interested in.
  this->Connect(esProxy, this->SelectionRepresentation, "Input", 1);
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
    this->OptionsProxy->SetTable(vtkTable::SafeDownCast(this->GetOutput()));
    }

  return true;
}

//----------------------------------------------------------------------------
vtkChartXY* vtkSMXYChartRepresentationProxy::GetChart()
{
  if (this->ChartViewProxy)
    {
    return vtkChartXY::SafeDownCast(this->ChartViewProxy->GetChart());
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
  if (!chartView || chartView == this->ChartViewProxy)
    {
    return false;
    }

  this->ChartViewProxy = chartView;
  this->OptionsProxy->SetChart(chartView->GetChart());
  this->OptionsProxy->SetTableVisibility(this->Visibility != 0);
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMXYChartRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMXYChartViewProxy* chartView = vtkSMXYChartViewProxy::SafeDownCast(view);
  if (!chartView || chartView != this->ChartViewProxy)
    {
    return false;
    }

  this->OptionsProxy->RemovePlotsFromChart();
  this->OptionsProxy->SetChart(0);
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    this->OptionsProxy->SetTableVisibility(visible != 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);

  // Update our selection
  this->SelectionRepresentation->Update(view);

  if (this->GetChart())
    {
    vtkSelection *sel =
        vtkSelection::SafeDownCast(this->SelectionRepresentation->GetOutput());
    this->AnnLink->SetCurrentSelection(sel);
    this->GetChart()->SetAnnotationLink(AnnLink);
    }

  // Set the table, in case it has changed.
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
