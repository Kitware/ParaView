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
#include "vtkPen.h"
#include "vtkTable.h"
#include "vtkAnnotationLink.h"
#include "vtkSelection.h"
#include "vtkStringArray.h"
#include "vtkStringList.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMParallelCoordinatesRepresentationProxy);
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

  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->SetInput(0);
    this->GetChart()->SetVisible(false);
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
    if (this->GetChart())
      {
      this->GetChart()->SetVisible(visible != 0);
      }
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

    // Set the table, in case it has changed.
    this->GetChart()->GetPlot(0)
        ->SetInput(vtkTable::SafeDownCast(this->GetOutput()));
    }

  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp || !svp->GetInformationOnly() ||
      !vtkTable::SafeDownCast(this->GetOutput()))
    {
    return;
    }

  vtkTable* table = vtkTable::SafeDownCast(this->GetOutput());

  bool skip = false;
  const char* propertyName = this->GetPropertyName(prop);
  vtkSmartPointer<vtkStringList> newValues = vtkSmartPointer<vtkStringList>::New();

  int numberOfColumns = table->GetNumberOfColumns();
  for (int i = 0; i < numberOfColumns; ++i)
    {
    const char* seriesName = table->GetColumnName(i);
    if (!seriesName)
      {
      continue;
      }

    newValues->AddString(seriesName);

    if (strcmp(propertyName, "SeriesVisibilityInfo") == 0)
      {
      if (i < 10)
        {
        newValues->AddString("1");
        }
      else
        {
        newValues->AddString("0");
        }
      }
    else if (strcmp(propertyName, "LabelInfo") == 0)
      {
      newValues->AddString(seriesName);
      }
    else if (strcmp(propertyName, "LineThicknessInfo") == 0)
      {
      newValues->AddString("2");
      }
    else if (strcmp(propertyName, "ColorInfo") == 0)
      {
      newValues->AddString("0");
      newValues->AddString("0");
      newValues->AddString("0");
      }
    else if (strcmp(propertyName, "OpacityInfo") == 0)
      {
      newValues->AddString("0.10");
      }
    else if (strcmp(propertyName, "LineStyleInfo") == 0)
      {
      newValues->AddString("1");
      }
    else
      {
      skip = true;
      break;
      }
    }
  if (!skip)
    {
    svp->SetElements(newValues);
    }
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

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetSeriesVisibility(
    const char* name, int visible)
{
  if (this->GetChart())
    {
    this->GetChart()->SetColumnVisibility(name, visible != 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetLabel(const char*,
                                                           const char*)
{

}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetLineThickness(int value)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetLineStyle(int value)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetLineType(value);
    }
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetColor(double r, double g,
                                                           double b)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetColorF(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkSMParallelCoordinatesRepresentationProxy::SetOpacity(double opacity)
{
  if (this->GetChart())
    {
    this->GetChart()->GetPlot(0)->GetPen()->SetOpacityF(opacity);
    }
}
