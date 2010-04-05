/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParallelCoordinatesRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMParallelCoordinatesRepresentationProxy.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMXYChartViewProxy.h"
#include "vtkContextView.h"
#include "vtkChartParallelCoordinates.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"

vtkStandardNewMacro(vtkSMParallelCoordinatesRepresentationProxy);
vtkCxxRevisionMacro(vtkSMParallelCoordinatesRepresentationProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMParallelCoordinatesRepresentationProxy::vtkSMParallelCoordinatesRepresentationProxy()
{
  this->Visibility = 1;
  this->AnnLink = vtkAnnotationLink::New();
}

//----------------------------------------------------------------------------
vtkSMParallelCoordinatesRepresentationProxy::~vtkSMParallelCoordinatesRepresentationProxy()
{
  this->AnnLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkSMParallelCoordinatesRepresentationProxy::BeginCreateVTKObjects()
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
void vtkSMParallelCoordinatesRepresentationProxy::CreatePipeline(vtkSMSourceProxy* input,
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
bool vtkSMParallelCoordinatesRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  // The reduction type for all chart representation is TABLE_MERGE since charts
  // always deliver tables.
  this->SetReductionType(vtkSMClientDeliveryRepresentationProxy::TABLE_MERGE);

  return true;
}

//----------------------------------------------------------------------------
vtkChartParallelCoordinates* vtkSMParallelCoordinatesRepresentationProxy::GetChart()
{
  if (this->ChartViewProxy)
    {
    return vtkChartParallelCoordinates::SafeDownCast(
        this->ChartViewProxy->GetChart());
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
bool vtkSMParallelCoordinatesRepresentationProxy::AddToView(vtkSMViewProxy* view)
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

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMParallelCoordinatesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMXYChartViewProxy* chartView = vtkSMXYChartViewProxy::SafeDownCast(view);
  if (!chartView || chartView != this->ChartViewProxy)
    {
    return false;
    }

  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    }
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::Update(vtkSMViewProxy* view)
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
  this->GetChart()->GetPlot(0)->SetInput(vtkTable::SafeDownCast(this->GetOutput()));

  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
int vtkSMParallelCoordinatesRepresentationProxy::GetNumberOfSeries()
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
const char* vtkSMParallelCoordinatesRepresentationProxy::GetSeriesName(int col)
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
