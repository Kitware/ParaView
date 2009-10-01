/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartRepresentationProxy.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkQtChartNamedSeriesOptionsModel.h"
#include "vtkQtChartRepresentation.h"
#include "vtkQtChartView.h"
#include "vtkSMChartNamedOptionsModelProxy.h"
#include "vtkSMChartViewProxy.h"

vtkStandardNewMacro(vtkSMChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMChartRepresentationProxy, "1.4");
//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::vtkSMChartRepresentationProxy()
{
  this->VTKRepresentation = vtkQtChartRepresentation::New();
  this->Visibility = 1;

  this->UseIndexForXAxis = true;
  this->XSeriesName = 0;
}

//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::~vtkSMChartRepresentationProxy()
{
  this->SetXSeriesName(0);
  this->VTKRepresentation->Delete();
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMChartRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  // The reduction type for all chart representation is TABLE_MERGE since charts
  // always deliver tables.
  this->SetReductionType(vtkSMClientDeliveryRepresentationProxy::TABLE_MERGE);

  this->OptionsProxy = vtkSMChartNamedOptionsModelProxy::SafeDownCast(
    this->GetSubProxy("SeriesOptions"));
  this->OptionsProxy->CreateObjects(this->VTKRepresentation);
  this->VTKRepresentation->SetOptionsModel(
    this->OptionsProxy->GetOptionsModel());
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMChartRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  vtkSMChartViewProxy* chartView = vtkSMChartViewProxy::SafeDownCast(view);
  if (!chartView)
    {
    return false;
    }

  this->ChartViewProxy = chartView;
  if (this->Visibility)
    {
    this->ChartViewProxy->GetChartView()->AddRepresentation(
      this->VTKRepresentation);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMChartRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMChartViewProxy* chartView = vtkSMChartViewProxy::SafeDownCast(view);
  if (!chartView || chartView != this->ChartViewProxy)
    {
    return false;
    }

  if (this->Visibility && this->ChartViewProxy)
    {
    this->ChartViewProxy->GetChartView()->RemoveRepresentation(
      this->VTKRepresentation);
    }
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    if (this->ChartViewProxy)
      {
      if (this->Visibility)
        {
        this->ChartViewProxy->GetChartView()->AddRepresentation(
          this->VTKRepresentation);
        }
      else
        {
        this->ChartViewProxy->GetChartView()->RemoveRepresentation(
          this->VTKRepresentation);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  this->VTKRepresentation->SetInputConnection(
    this->GetOutput()->GetProducerPort());
  this->VTKRepresentation->Update();
  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
int vtkSMChartRepresentationProxy::GetNumberOfSeries()
{
  return this->VTKRepresentation->GetNumberOfSeries();
}

//----------------------------------------------------------------------------
const char* vtkSMChartRepresentationProxy::GetSeriesName(int series)
{
  return this->VTKRepresentation->GetSeriesName(series);
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::SetUseIndexForXAxis(bool use_index)
{
  this->UseIndexForXAxis = use_index;
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::SetXAxisSeriesName(const char* name)
{
  this->SetXSeriesName(name);
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::UpdateXSeriesName()
{
  if (!this->UseIndexForXAxis && this->XSeriesName && this->XSeriesName[0])
    {
    this->VTKRepresentation->SetKeyColumn(this->XSeriesName);
    }
  else
    {
    this->VTKRepresentation->SetKeyColumn(0);
    }
}

